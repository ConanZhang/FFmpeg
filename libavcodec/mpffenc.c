/**
 * Authors: Charles Khong and Conan Zhang
 * Date: March 2, 2015
 *
 * Description:
 *      Encoder for the MPFF image format.
 *
 *      Contains functions:
 *          mpff_encode_init: Begins the process of encoding an MPFF file
 *          mpff_encode_frame: The actual process of encoding the MPFF file
 *          mpff_encode_close: Ends the process of encoding an MPFF file
 *
 */ 
#include "mpff.h"

/** 
 * Initializes encoding of MPFF file
 *      
 *      Sets pixel depth and allocates memory for the image
 *
 */
static av_cold int mpff_encode_init(AVCodecContext *avctx){
    // Use 8 bits of color per pixel
    avctx->bits_per_coded_sample = 8;

    // Allocate memory for AVFrame and error check if we have enough memory or not
    avctx->coded_frame = av_frame_alloc();
    if (!avctx->coded_frame)
        return AVERROR(ENOMEM);

    return 0;
}

/** 
 * Process of encoding an MPFF image into a file 
 * 
 * Sets image properties, calculates data sizes, writes header, and writes image.
 *
 */
static int mpff_encode_frame(AVCodecContext *avctx, AVPacket *pkt, const AVFrame *pict, int *got_packet)
{
    /* Forward variable declarations */
    int file_size, header_size, image_size, file_header_size, info_header_size;// Size properties in bytes
    int image_row_size, padding_row_size;// Image data stored in rows with bytes 
    int depth = avctx->bits_per_coded_sample;// Amount of memory used to store pixels
    int output_size;// Error checking for enough memory for the output buffer
    int i;// Looping variable for writing image to buffer

    const AVFrame * const picture = pict;// The raw image we were given
    uint8_t *image_data, *buffer;// Pointers to data

    /* Set properties */
    // Our image format is simply an image
    avctx->coded_frame->pict_type = AV_PICTURE_TYPE_I;
    // Our image format has one key frame
    avctx->coded_frame->key_frame = 1;

    /* Calculate sizes */
    // Image data
    image_row_size = ((int64_t)avctx->width * (int64_t)depth + 7LL) >> 3LL;
    padding_row_size = (4 - image_row_size) & 3;
    image_size = avctx->height * (image_row_size + padding_row_size);

    // Header data
    file_header_size = 12;
    info_header_size = 14;

    // File data
    header_size = file_header_size + info_header_size;    
    file_size = image_size + header_size;

    /*Writing header data*/
    // Ensure our output buffer is large enough
    if ((output_size = ff_alloc_packet2(avctx, pkt, file_size)) < 0)
        return output_size;

    // Get buffer to write data to
    buffer = pkt->data;

    // Magic Number
    bytestream_put_byte(&buffer, 'M');                 
    bytestream_put_byte(&buffer, 'P');                   
    bytestream_put_byte(&buffer, 'F');
    bytestream_put_byte(&buffer, 'F');
    
    // Sizes
    bytestream_put_le32(&buffer, file_size);        // Total file size (header + image data)
    bytestream_put_le32(&buffer, header_size);      // Total header size 
    bytestream_put_le32(&buffer, info_header_size); // Info header size

    // Dimensions
    bytestream_put_le32(&buffer, avctx->width);     // Image width
    bytestream_put_le32(&buffer, avctx->height);    // Image height
    bytestream_put_le16(&buffer, depth);            // Pixel depth (bits per pixel)

    /* Writing image data to file */
    // Get image data 
    image_data = picture -> data[0];
    // Get buffer to store image in
    buffer = pkt->data + header_size;

    // Start writing image from top-left to bottom-right
    for(i = 0; i < avctx->height; i++){
        //Copy rows of image_data to our buffer 
        memcpy(buffer, image_data, image_row_size);
        //Move to padding row 
        buffer += image_row_size;

        //Put padding inbetween rows
        memset(buffer, 0, padding_row_size);
        //Move to next clean row after padding 
        buffer += padding_row_size;

        //Move cursor to beginning of image_data
        image_data += picture->linesize[0];
    }

    // Our file contains a key frame
    pkt->flags |= AV_PKT_FLAG_KEY;
    // Say that we have encoded a complete frame
    *got_packet = 1;

    return 0;
}

/** 
 * Ends the encoding process of MPFF file format 
 *
 *      Deallocates memory the image was contained in.
 *
 */
static av_cold int mpff_encode_close(AVCodecContext *avctx)
{
    av_frame_free(&avctx->coded_frame);
    return 0;
}

/** 
 * Structure of MPFF encoder
 *  
 *      Defines entry points for functions that contribute to the codec.
 */
AVCodec ff_mpff_encoder = {
    .name           = "mpff",
    .long_name      = NULL_IF_CONFIG_SMALL("MPFF image (a CS 3505 project)"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_MPFF,
    .init           = mpff_encode_init,
    .encode2        = mpff_encode_frame,
    .close          = mpff_encode_close,
    .pix_fmts       = (const enum AVPixelFormat[]){AV_PIX_FMT_RGB8, AV_PIX_FMT_NONE},
};
