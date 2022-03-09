#include "get_image.h"


int cam_close(int cam_fd)
{
    int buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    int ret = ioctl(cam_fd, VIDIOC_STREAMOFF, &buffer_type);
    if (ret < 0)
    {
        int errnum = errno;
        printf("Fail to set set stream off, err msg: %s\n", strerror(errnum));
        return -1;
    }

    ret = close(cam_fd); //disconnect camera
    if (ret < 0)
    {
        int errnum = errno;
        printf("Fail to close cam, err msg: %s\n", strerror(errnum));
        return -1;
    }
    return 0;
}
int cam_init(int IMAGE_WIDTH, int IMAGE_HEIGHT)
{
    int i;
    int ret;
    int cam_fd;
    int sel = 0;

    struct v4l2_format format;
    struct v4l2_buffer video_buffer[BUFFER_COUNT];

    // Open Camera
    cam_fd = open(VIDEO_DEVICE, O_RDWR); //connect camera

    if (cam_fd < 0)
    {
        int errnum = errno;
        printf("Fail to open Camera, err msg: %s\n", strerror(errnum));
        return -1;
    }

    ret = ioctl(cam_fd, VIDIOC_S_INPUT, &sel); //setting video input
    if (ret < 0)
    {
        int errnum = errno;
        printf("Fail to set video input, err msg: %s\n", strerror(errnum));
        return -1;
    }

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = SENSOR_COLORFORMAT; //V4L2_PIX_FMT_SGRBG10;//10bit raw
    format.fmt.pix.width = IMAGE_WIDTH;              //resolution
    format.fmt.pix.height = IMAGE_HEIGHT;
    ret = ioctl(cam_fd, VIDIOC_TRY_FMT, &format);

    if (ret != 0)
    {
        int errnum = errno;
        printf("Fail to try fmt, err msg: %s\n", strerror(errnum));
        return ret;
    }

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(cam_fd, VIDIOC_S_FMT, &format);
    if (ret != 0)
    {
        int errnum = errno;
        printf("Fail to set fmt, err msg: %s\n", strerror(errnum));
        return ret;
    }

    struct v4l2_requestbuffers req;
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(cam_fd, VIDIOC_REQBUFS, &req);
    if (ret != 0)
    {
        int errnum = errno;
        printf("Fail to req buf, err msg: %s\n", strerror(errnum));
        return ret;
    }
    if (req.count < BUFFER_COUNT)
    {
        int errnum = errno;
        printf("Req buf < buffer_count, err msg: %s\n", strerror(errnum));
        return ret;
    }

    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.type = req.type;
    buffer.memory = V4L2_MEMORY_MMAP;
    for (i = 0; i < req.count; i++)
    {
        buffer.index = i;
        ret = ioctl(cam_fd, VIDIOC_QUERYBUF, &buffer);
        if (ret != 0)
        {
            int errnum = errno;
            printf("Fail to Query buf, err msg: %s\n", strerror(errnum));
            return ret;
        }

        video_buffer_ptr[i] = (uint8_t *)mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, cam_fd, buffer.m.offset);
        if (video_buffer_ptr[i] == MAP_FAILED)
        {
            int errnum = errno;
            printf("Fail to mapping, err msg: %s\n", strerror(errnum));
            return ret;
        }

        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        ret = ioctl(cam_fd, VIDIOC_QBUF, &buffer);
        if (ret != 0)
        {
            int errnum = errno;
            printf("Fail to Q buf, err msg: %s\n", strerror(errnum));
            return ret;
        }
    }

    int buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(cam_fd, VIDIOC_STREAMON, &buffer_type);
    if (ret != 0)
    {
        int errnum = errno;
        printf("Fail to Streamon, err msg: %s\n", strerror(errnum));
        return ret;
    }

    return cam_fd;
}


void cvt_ByteOrder(uint8_t *new_file, uint8_t *raw_file, int raw_buffer_size, int pixel_bit)
{
    switch (pixel_bit)
    {
    case 10:
        for (int i = 0; i < raw_buffer_size; i += 2)
        {
            new_file[i] = ((raw_file[i + 1] & 0x3f) << 2) | (raw_file[i] >> 6);
            new_file[i + 1] = 0 | (raw_file[i + 1] >> 6);
        }
        break;
    case 12:
        for (int i = 0; i < raw_buffer_size; i += 2)
        {
            new_file[i] = ((raw_file[i + 1] & 0x0f) << 4) | (raw_file[i] >> 4);
            new_file[i + 1] = 0 | (raw_file[i + 1] >> 4);
        }
        break;
    case 14:
        for (int i = 0; i < raw_buffer_size; i += 2)
        {
            new_file[i] = ((raw_file[i + 1] & 0x03) << 6) | (raw_file[i] >> 2);
            new_file[i + 1] = 0 | (raw_file[i + 1] >> 2);
        }
        break;
    }
}

int cam_get_image(uint8_t *out_buffer, int out_buffer_size, int cam_fd)
{
    int ret;
    struct v4l2_buffer buffer;

    memset(&buffer, 0, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = BUFFER_COUNT;
    ret = ioctl(cam_fd, VIDIOC_DQBUF, &buffer);
    if (ret != 0)
        return ret;

    if (buffer.index < 0 || buffer.index >= BUFFER_COUNT)
        return ret;

    memcpy(out_buffer, video_buffer_ptr[buffer.index], out_buffer_size);

    ret = ioctl(cam_fd, VIDIOC_QBUF, &buffer);
    if (ret != 0)
        return ret;

    return 0;
}
