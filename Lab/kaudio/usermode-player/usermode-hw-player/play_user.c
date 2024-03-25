#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <alsa/asoundlib.h>
#define SND_CARD "default"

static FILE* fp2;
static float delay_s;

// NOTE use sizes from STDINT
// NOTE verify data alignment!
struct wave_header
{
  uint32_t chunkID;
  uint32_t chunkSize;
  uint32_t format;

  uint32_t subchunk1ID;
  uint32_t subchunk1Size;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t SampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;

  uint32_t subchunk2ID;
  uint32_t subchunk2Size;
};

void pr_usage(char* pname)
{
  printf("usage: %s WAV_FILE\n", pname);
}

/* @brief Read WAVE header
   @param fp file pointer
   @param dest destination struct
   @return 0 on success, < 0 on error */
int read_wave_header(FILE* fp, struct wave_header* dest)
{
  if (!dest || !fp)
    {
      return -ENOENT;
    }

  rewind(fp); //Resets the file pointer

  struct stat st;
  fstat(fileno(fp), &st);
  if(!(S_ISREG(st.st_mode) && st.st_size > 44))
    return -1;


  //read header
  int x = 0;
  x=fseek(fp, 0, SEEK_SET);
  x=fread(&(dest->chunkID), sizeof(uint32_t), 1, fp);  
  x=fseek(fp, 4, SEEK_SET);
  x=fread(&(dest->chunkSize), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 8, SEEK_SET);
  x=fread(&(dest->format), sizeof(uint32_t), 1, fp);

  x=fseek(fp, 12, SEEK_SET);
  x=fread(&(dest->subchunk1ID), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 16, SEEK_SET);
  x=fread(&(dest->subchunk1Size), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 20, SEEK_SET);
  x=fread(&(dest->audioFormat), sizeof(uint16_t), 1, fp);
  x=fseek(fp, 22, SEEK_SET);
  x=fread(&(dest->numChannels), sizeof(uint16_t), 1, fp);
  x=fseek(fp, 24, SEEK_SET);
  x=fread(&(dest->SampleRate), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 28, SEEK_SET);
  x=fread(&(dest->byteRate), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 32, SEEK_SET);
  x=fread(&(dest->blockAlign), sizeof(uint16_t), 1, fp);
  x=fseek(fp, 34, SEEK_SET);
  x=fread(&(dest->bitsPerSample), sizeof(uint16_t), 1, fp);

  x=fseek(fp, 36, SEEK_SET);
  x=fread(&(dest->subchunk2ID), sizeof(uint32_t), 1, fp);
  x=fseek(fp, 40, SEEK_SET);
  x=fread(&(dest->subchunk2Size), sizeof(uint32_t), 1, fp);

  //printf("Expected file size: %d, Actual file size: %ld\n", (dest->chunkSize + 8), st.st_size);

  return !((dest->chunkSize + 8) == st.st_size);
}

/* @brief Parse WAVE header and print parameters
   @param hdr a struct wave_header variable
   @return 0 on success, < 0 on error or if not WAVE file*/
int parse_wave_header(struct wave_header hdr)
{
  // verify that this is a RIFF file header
  if((hdr.chunkID != (0x46464952)))
    return -1;
  //verify that this is WAVE file
  if(hdr.format != (0x45564157))
    return -1;

  if(hdr.audioFormat != 1)
    return -1;

  //print out information: number of channels, sample rate, total size
  printf("Number of channels: %d, Sample Rate: %dHz, Total Size: %d bytes\n", hdr.numChannels, hdr.SampleRate, (hdr.chunkSize + 8));

  return 0;
}

/* @brief Transmit a word (put into FIFO)
   @param word a 32-bit word */
void write_word(int32_t word)
{
  //int x = system("echo \"hello\">/dev/echo0");
  fwrite(&word, sizeof(word), 1, fp2);
  //;
}

/* @brief Build a 32-bit audio word from a buffer
   @param hdr WAVE header
   @param buf a byte array
   @return 32-bit word */
uint32_t audio_word_from_buf(struct wave_header hdr, int8_t* buf)
{
  //build word depending on bits per sample, etc
  uint32_t audio_word = 0;
  //printf("BPS: %d\n", hdr.bitsPerSample);

  for(int i = 0 ; i < hdr.bitsPerSample/8; i++)
    audio_word |= (uint32_t)((buf[i] + 127) << (8*((24/hdr.bitsPerSample-1) - i)));
  //for(int i = 0 ; i < 24/hdr.bitsPerSample; i++)
    //audio_word |= (uint32_t)buf[i] << (8*((24/hdr.bitsPerSample-1) - i));
  return audio_word << 8;
}

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
            if(j%4 == 0)
              printf(" ");
        }
    }
    puts("\n");
}

/* @brief Play sound samples
   @param fp file pointer
   @param hdr WAVE header
   @param sample_count how many samples to play or -1 plays to end of file
   @param start starting point in file for playing
   @return 0 if successful, < 0 otherwise */
int play_wave_samples(FILE* fp,
                      struct wave_header hdr,
                      int sample_count,
                      unsigned int start)
{
  if (!fp)
    {
      return -EINVAL;
    }

  // NOTE reject if number of channels is not 1 or 2
  if(hdr.numChannels != 1 && hdr.numChannels !=2)
    return -EINVAL;

  // calculate starting point and move there
  int x=fseek(fp, 44 + start, SEEK_SET);

  // continuously read frames/samples and use fifo_transmit_word to
  //      simulate transmission
  int8_t chunk = 0;
  int8_t buf[(hdr.bitsPerSample/8) * hdr.numChannels];
  int8_t lbuf[(hdr.bitsPerSample/8)];
  int8_t rbuf[(hdr.bitsPerSample/8)];

  if(sample_count == -1) //Play the whole file through
  {
    sample_count = (hdr.chunkSize + 8) - start;
  }
  int i = 0;
  while (sample_count > 0)
  {
    // read chunk (whole frame)
    x=fread(buf, sizeof(int8_t), ((hdr.bitsPerSample/8)) * hdr.numChannels, fp);

    if(hdr.numChannels == 2) //Seperate into two different buffers for left and right, for 2-channel audio
    {
      for(int i = 0; i < (hdr.bitsPerSample/8) * hdr.numChannels; i+=2)
      {
        lbuf[i] = buf[i];
        rbuf[i] = buf[i + 1];
      }

      write_word(audio_word_from_buf(hdr, lbuf));
      write_word(audio_word_from_buf(hdr, rbuf));
      sample_count -= 1;
    }
    else
    {
      write_word(audio_word_from_buf(hdr, buf)); //For the left channel
      write_word(audio_word_from_buf(hdr, buf)); //For the right channel
    }
    //write samples properly independently if file is mono or stereo
    //printf("Sample %d: 0x%X\n", (sample_count), audio_word_from_buf(hdr, buf));
    //--printf("TDFV: 0x%X\n", read_reg(fifo, TDFV_OFFSET));
    //--printf("ISR: ");
    //--uint32_t isr= read_reg(fifo, ISR_OFFSET);
    //--printBits(sizeof(isr), &isr);
    //--printf("Sample#: %d done\n", i);
    
    sample_count -= 1;
    i += 2;
  }

  return 0;
}

int i2s_enable_tx()
{
  return system("echo \"1\"> /sys/devices/soc0/amba_pl/77600000.axi_i2s_adi/tx_enabled");
}

int i2s_disable_tx()
{
  return system("echo \"0\"> /sys/devices/soc0/amba_pl/77600000.axi_i2s_adi/tx_enabled");
}

int configure_codec(unsigned int sample_rate, 
                    snd_pcm_format_t format, 
                    snd_pcm_t* handle,
                    snd_pcm_hw_params_t* params)
{
  int err;

  // initialize parameters 
  err = snd_pcm_hw_params_any(handle, params);
  if (err < 0)
  {
      // failed, handle and return...
  }

  // set format
  // NOTE: the codec only supports one audio format, this should be constant
  //       and not read from the WAVE file. You must convert properly to this 
  //       format, regardless of the format in your WAVE file 
  //       (bits per sample and alignment).
  snd_pcm_hw_params_set_format(handle, params, format);

  // set channel count
  snd_pcm_hw_params_set_channels(handle, params, 2);

  // set sample rate
  printf("Expected Rate: %d\n", sample_rate);
  snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, 0);
  //unsigned int *rate;
  //int *dir;
  int actual_rate = 0;
  //*dir = 0;
  snd_pcm_hw_params_get_rate(params, &actual_rate, NULL);
  printf("Actual Rate: %d\n", sample_rate);

  // write parameters to device
  snd_pcm_hw_params(handle, params);

  return 0;
}


int main(int argc, char** argv)
{

  snd_pcm_t *handle;
  snd_pcm_hw_params_t *hwparams;
  int err;
  FILE* fp;
  struct wave_header hdr;
  // placeholder variables, use values you read from your WAVE file
  unsigned int sample_rate;
  snd_pcm_format_t sound_format;

  // check number of arguments
  if (argc < 2)
    {
      // fail, print usage
      pr_usage(argv[0]);
      printf("Error: Expected second argument (.WAV file)\n");
      return 1;
    }

  // TODO read WAVE file, find out parameters, etc (from pre-lab 4a)
  // open file
  fp = fopen(argv[1], "r");
  fp2 = fopen("/dev/zedaudio0", "w");

  // read file header
  if(read_wave_header(fp, &hdr) != 0)
  {
    printf("Error: Incorrect File Format\n");
    return -1;
  }

  // parse file header, verify that is wave
  if(parse_wave_header(hdr) != 0)
  {
    printf("Error: Incorrect WAV format\n");
    return -1;
  }

  sample_rate = hdr.SampleRate;
  delay_s = 924.0/sample_rate;
  sound_format = SND_PCM_FORMAT_U32_LE;

  // allocate HW parameter data structures
  snd_pcm_hw_params_alloca(&hwparams);

  err = i2s_enable_tx();
  if(err != 0)
  {
    printf("Error: Unable to start I2S\n");
    return -1;
  }

  // open device (TX)
  err = snd_pcm_open(&handle, SND_CARD, SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0)
  {
      // failed, handle...
  }

  err = configure_codec(sample_rate, sound_format, handle, hwparams);
  if (err < 0)
  {
      // failed, handle...
  }

  // TODO play sound (from pre-lab 5a)
  play_wave_samples(fp, hdr, -1, 0);
  
  // TODO do rest of cleanup
  snd_pcm_close(handle);
  fclose(fp);
  fclose(fp2);
  
  err = i2s_disable_tx();
  return 0;

  
  return 0;
}