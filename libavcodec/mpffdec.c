/**
 * Authors: Charles Khong and Conan Zhang
 * Date: March 2, 2015
 *
 * Description:
 *      Decoder for the MPFF image format.
 *
 *      Contains functions:
 *          mpff_decode_frame: Process of decoding an MPFF file. 
 *
 */

#include "mpff.h"

/*
 * Process of decoding an MPFF file
 * 
 * Checks that the headers match the information about the data
 * And sets the AVCodecContext to contain the raw image data from the MPFF file
 */
static int mpff_decode_frame(AVCodecContext *avctx,
                            void *data, int *got_frame,
                            AVPacket *avpkt)
{
    const uint8_t *buffer = avpkt->data;                   // The image data from the MPFF file
    int buffer_size       = avpkt->size;                   // The size of the MPFF file
    AVFrame *picture      = data;                          // The frame of the image we are decoding into
    unsigned int file_size, header_size, info_header_size; // File size and header size (file header + info header)
    int width, height;                                     // The dimensions of the MPFF image
    unsigned int depth;                                    // The bits per pixel of the image (8 bits) 
    int buffer_linesize, linesize;                         // The line sizes for stepping through the image/frame
    int ret, i;                                            // The frame buffer, i is the index for a for loop
    uint8_t *image_array;                                  // The image array to write to for the frame
    const uint8_t *buffer_start = buffer;                  // The beginning of the MPFF file buffer

    /* Error checking*/
    // Buffer size
    if (buffer_size < 12)
    {
        av_log(avctx, AV_LOG_ERROR, "buffer size too small (%d)\n", buffer_size);
        return AVERROR_INVALIDDATA;
    }

    // Magic number
    if (bytestream_get_byte(&buffer) != 'M' ||
        bytestream_get_byte(&buffer) != 'P' ||
        bytestream_get_byte(&buffer) != 'F' ||
	bytestream_get_byte(&buffer) != 'F') 
    {
        av_log(avctx, AV_LOG_ERROR, "Incorrect magic number. Expected MPFF\n");
        return AVERROR_INVALIDDATA;
    }

    // Get image + header size
    file_size = bytestream_get_le32(&buffer);

    // Checking buffer vs. file size
    if (buffer_size < file_size) 
    {
        av_log(avctx, AV_LOG_ERROR, "Expected more data than available: (%d < %u). Attempting to decode anyway\n",
               buffer_size, file_size);
        file_size = buffer_size;
    }

    // Total size of the header
    header_size  = bytestream_get_le32(&buffer); 

    // Size of the info header
    info_header_size = bytestream_get_le32(&buffer);

    // Header sizes added together shouldn't be bigger than what we expect
    if (info_header_size + 12LL > header_size) 
    {
        av_log(avctx, AV_LOG_ERROR, "Invalid header size: %u\n", header_size);
        return AVERROR_INVALIDDATA;
    }

    // Sometimes file size is set to some headers size, set a real size in that case 
    if (file_size == 12 || file_size == info_header_size + 12)
        file_size = buffer_size - 2;

    // If the header is larger than the actual file
    if (file_size <= header_size) 
    {
        av_log(avctx, AV_LOG_ERROR,
               "Declared file size is less than header size (%u < %u)\n",
               file_size, header_size);
        return AVERROR_INVALIDDATA;
    }

    // Get dimensions of image
    width  = bytestream_get_le32(&buffer);
    height = bytestream_get_le32(&buffer);
    depth  = bytestream_get_le16(&buffer);

    // Set up raw image dimensions
    // Note: The height should be stored as a positive number
    avctx->width   = width;
    avctx->height  = height > 0 ? height : -height;
    avctx->pix_fmt = AV_PIX_FMT_RGB8;

    // Get buffer for the frame and if it is negative than return the error code
    if ((ret = ff_get_buffer(avctx, picture, 0)) < 0)
        return ret;

    // Set up picture to be an image (instead of video)
    picture->pict_type = AV_PICTURE_TYPE_I;
    picture->key_frame = 1;

    // Set buffer to after the header where we want to write the image
    buffer = buffer_start + header_size;

    /* Line size in file multiple of 4 */
    buffer_linesize = ((avctx->width * depth + 31) / 8) & ~3;

    // Set beginning position and linesize to write to
    image_array      = picture->data[0];
    linesize         = picture->linesize[0];

    // Start the buffer at the beginning of the image data
    buffer = buffer_start + header_size;

    // Copy the image data from the MPFF file into the output image array
    for (i = 0; i < avctx->height; i++) 
    {
      // Copy a MPFF's linesize worth of bytes (pixels + padding bytes)
      // into the image array
      memcpy(image_array, buffer, buffer_linesize);

      // Increment to the next row of the image buffer
      buffer += buffer_linesize;

      // Increment to write to the next row of the image array
      image_array += linesize;
    }

    // Indicate that we decoded without any errors
    *got_frame = 1;

    return buffer_size;
}

/*
 * The structural functionalities of an .mpff decoder.
 */
AVCodec ff_mpff_decoder = {
    .name           = "mpff",
    .long_name      = NULL_IF_CONFIG_SMALL("MPFF Image (a CS 3505 project)"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPFF,
    .decode         = mpff_decode_frame,
    .capabilities   = CODEC_CAP_DR1,
};
