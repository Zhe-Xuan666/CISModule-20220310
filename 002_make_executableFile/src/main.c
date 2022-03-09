#include "get_image.h"
#include "myFuncts.h"

/*read_ini*/
char buffIni[40];
char iniFile[20];
int IMAGE_WIDTH;
int IMAGE_HEIGHT;
int IMAGE_SIZE;
/*read_ini*/

#define STOP_NUM 5 //REPEAT STOP NUM

int main()
{
    int cam_fd;
    FILE *f;
    uint8_t *raw_buffer, *cvt_buffer, *tmp_buffer;
    uint8_t file_header[16];
    char tmp[64] = {"---\n"};
    int frame_num = 1, repeat_cnt = 0;

    /*Read ini*/
    printf("Start calling <config.ini> to read raw data:\r\n");
    strcpy(iniFile, "config.ini");

    if (!initFuncts(iniFile))
    {
        puts("initFuncts error");
        return EXIT_FAILURE;
    }

    puts("initFuncts OK");
    /*Read ini*/

    /*Set buffer size*/
    raw_buffer = (uint8_t *)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * 2 * sizeof(uint8_t));
    cvt_buffer = (uint8_t *)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * 2 * sizeof(uint8_t));
    tmp_buffer = (uint8_t *)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * 2 * sizeof(uint8_t));

    /*init cam*/
    cam_fd = cam_init(IMAGE_WIDTH, IMAGE_HEIGHT);
    if (cam_fd == -1)
    {
        printf("fail to init\n");
        free(raw_buffer);
        free(cvt_buffer);
        free(tmp_buffer);
        return -1;
    }
    printf("Init OK, sleep for 5 sec\n");
    // sleep(5);
    /*init cam*/

    while (1)
    {
        /*Get Data*/
        if (cam_get_image(raw_buffer, IMAGE_SIZE, cam_fd) != 0)
        {
            printf("error get image\n");
            goto end;
        }
        /*Get Data*/

        /*Show Data*/
        if (repeat_cnt < STOP_NUM)
        {
            for (int i = 0; i < 16; i++)
                sprintf(&tmp[strlen(tmp)], "%02x ", raw_buffer[i]);
            printf("%s\r\n", tmp);
            memset(tmp, 0, 16);
        }
        /*Show Data*/

        /*Convert Data Byte Order*/
        cvt_ByteOrder(cvt_buffer, raw_buffer, IMAGE_SIZE, 12);
        /*Convert Data Byte Order*/

        /*Detect DataChange or not*/
        if (frame_num == 1)
        {
            memcpy(tmp_buffer, cvt_buffer, IMAGE_WIDTH * IMAGE_HEIGHT * 2 * sizeof(uint8_t));
        }
        else
        {
            if (strcmp((const char *)tmp_buffer, (const char *)cvt_buffer) == 0)
            {
                    printf("Raw data has not change %d times \n", ++repeat_cnt);
            }
            else
            {
                repeat_cnt = 0;
                printf("new data\n");
            }
            memcpy(tmp_buffer, cvt_buffer, IMAGE_WIDTH * IMAGE_HEIGHT * 2 * sizeof(uint8_t));
        }
        /*Detect DataChange or not*/

        /*Write File*/
        if (repeat_cnt < STOP_NUM)
        {
            f = fopen("./Pic/Test_Output.raw", "wb");
            if (f == NULL)
            {
                printf("Fail to open file\n");
                goto end;
            }
            memset(file_header, 0, 16);
            file_header[8] = IMAGE_WIDTH % 256;
            file_header[9] = (int)IMAGE_WIDTH / 256;
            file_header[12] = IMAGE_HEIGHT % 256;
            file_header[13] = (int)IMAGE_HEIGHT / 256;
            fwrite(file_header, 1, 16, f);
            fseek(f, 16, SEEK_SET);
            fwrite(cvt_buffer, 1, IMAGE_SIZE, f);
            fclose(f);
        }
        else
        {
            printf("No new data, Stop Write File.\n");
        }
        /*Write File*/

        // next Frame
        frame_num++;

    }

end:
    cam_close(cam_fd);
    free(raw_buffer);
    free(cvt_buffer);
    free(tmp_buffer);

    return 0;
}
