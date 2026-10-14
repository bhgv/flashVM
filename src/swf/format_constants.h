/* FlashVM: SWF tag identifiers and version compatibility boundaries. */
#ifndef FVM_SWF_FORMAT_CONSTANTS_H
#define FVM_SWF_FORMAT_CONSTANTS_H /* Header guard. */
/* SWF end tag identifier. */
#define FVM_SWF_TAG_END 0U
/* SWF show frame tag identifier. */
#define FVM_SWF_TAG_SHOW_FRAME 1U
/* SWF define shape tag identifier. */
#define FVM_SWF_TAG_DEFINE_SHAPE 2U
/* SWF place object tag identifier. */
#define FVM_SWF_TAG_PLACE_OBJECT 4U
/* SWF remove object tag identifier. */
#define FVM_SWF_TAG_REMOVE_OBJECT 5U
/* SWF define bits tag identifier. */
#define FVM_SWF_TAG_DEFINE_BITS 6U
/* SWF jpeg tables tag identifier. */
#define FVM_SWF_TAG_JPEG_TABLES 8U
/* SWF set background color tag identifier. */
#define FVM_SWF_TAG_SET_BACKGROUND_COLOR 9U
/* SWF do action tag identifier. */
#define FVM_SWF_TAG_DO_ACTION 12U
/* SWF define bits lossless tag identifier. */
#define FVM_SWF_TAG_DEFINE_BITS_LOSSLESS 20U
/* SWF define bits jpeg2 tag identifier. */
#define FVM_SWF_TAG_DEFINE_BITS_JPEG2 21U
/* SWF define shape2 tag identifier. */
#define FVM_SWF_TAG_DEFINE_SHAPE2 22U
/* SWF place object2 tag identifier. */
#define FVM_SWF_TAG_PLACE_OBJECT2 26U
/* SWF remove object2 tag identifier. */
#define FVM_SWF_TAG_REMOVE_OBJECT2 28U
/* SWF define shape3 tag identifier. */
#define FVM_SWF_TAG_DEFINE_SHAPE3 32U
/* SWF define bits jpeg3 tag identifier. */
#define FVM_SWF_TAG_DEFINE_BITS_JPEG3 35U
/* SWF define bits lossless2 tag identifier. */
#define FVM_SWF_TAG_DEFINE_BITS_LOSSLESS2 36U
/* SWF define sprite tag identifier. */
#define FVM_SWF_TAG_DEFINE_SPRITE 39U
/* SWF frame label tag identifier. */
#define FVM_SWF_TAG_FRAME_LABEL 43U
/* SWF export assets tag identifier. */
#define FVM_SWF_TAG_EXPORT_ASSETS 56U
/* SWF do init action tag identifier. */
#define FVM_SWF_TAG_DO_INIT_ACTION 59U
/* SWF file attributes tag identifier. */
#define FVM_SWF_TAG_FILE_ATTRIBUTES 69U
/* SWF metadata tag identifier. */
#define FVM_SWF_TAG_METADATA 77U
/* SWF version 1 compatibility boundary. */
#define FVM_SWF_VERSION_1 1U
/* SWF version 2 compatibility boundary. */
#define FVM_SWF_VERSION_2 2U
/* SWF version 3 compatibility boundary. */
#define FVM_SWF_VERSION_3 3U
/* SWF version 4 compatibility boundary. */
#define FVM_SWF_VERSION_4 4U
/* SWF version 5 compatibility boundary. */
#define FVM_SWF_VERSION_5 5U
/* SWF version 6 compatibility boundary. */
#define FVM_SWF_VERSION_6 6U
/* SWF version 7 compatibility boundary. */
#define FVM_SWF_VERSION_7 7U
/* SWF version 8 compatibility boundary. */
#define FVM_SWF_VERSION_8 8U
/* Solid fill style discriminator. */
#define FVM_SWF_FILL_SOLID 0x00U
/* Linear gradient fill style. */
#define FVM_SWF_FILL_LINEAR_GRADIENT 0x10U
/* Radial gradient fill style. */
#define FVM_SWF_FILL_RADIAL_GRADIENT 0x12U
/* Smoothed repeating bitmap fill. */
#define FVM_SWF_FILL_BITMAP_REPEAT 0x40U
/* Smoothed clipped bitmap fill. */
#define FVM_SWF_FILL_BITMAP_CLIPPED 0x41U
/* Unsmooth/repeating bitmap fill available under the newer version policy. */
#define FVM_SWF_FILL_BITMAP_REPEAT_NEAREST 0x42U
/* Unsmooth/clipped bitmap fill available under the newer version policy. */
#define FVM_SWF_FILL_BITMAP_CLIPPED_NEAREST 0x43U
/* Lossless bitmap format using a palette and indexed pixels. */
#define FVM_SWF_LOSSLESS_COLORMAP 3U
/* Lossless bitmap format using packed 15-bit RGB pixels. */
#define FVM_SWF_LOSSLESS_RGB15 4U
/* Lossless bitmap format using 32-bit RGB/ARGB storage. */
#define FVM_SWF_LOSSLESS_RGB32 5U
/* Low six tag-header bits encode a short length or the long-length sentinel. */
#define FVM_SWF_TAG_LENGTH_MASK 63U
/* Tag identifier starts after the six short-length bits. */
#define FVM_SWF_TAG_CODE_SHIFT 6U
/* Authoring debugger password tag (legacy form). */
#define FVM_SWF_TAG_ENABLE_DEBUGGER 58U
/* Opaque authoring debugger identifier tag. */
#define FVM_SWF_TAG_DEBUG_ID 63U
/* Authoring debugger password tag with a reserved UI16 prefix. */
#define FVM_SWF_TAG_ENABLE_DEBUGGER2 64U
/* DebugID is exactly one 128-bit opaque identifier. */
#define FVM_SWF_DEBUG_ID_BYTES 16U
/* EnableDebugger2 begins with a reserved UI16, ignored by playback. */
#define FVM_SWF_DEBUGGER2_PREFIX_BYTES 2U
/* FileAttributes contains one UI32 flag word. */
#define FVM_SWF_FILE_ATTRIBUTES_BYTES 4U
/* Valid FileAttributes bits in the supported SWF layouts. */
#define FVM_SWF_FILE_ATTRIBUTES_VALID_FLAGS 0x79UL
/* Metadata is the neutral document capability supported by this loader. */
#define FVM_SWF_FILE_ATTRIBUTES_METADATA 0x10UL
#endif
