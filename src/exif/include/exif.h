/*
 * Copyright (C) 2013 KLab Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef EXIF_H_
#define EXIF_H_

#ifdef _WIN32
#define EXIF_API_EXPORT __declspec(dllexport)
#define EXIF_API_CALL
#else
#define EXIF_API_EXPORT /**< API export macro */
#define EXIF_API_CALL /**< API call macro */
#endif

#define EXIF_API_EXPORT_CALL EXIF_API_EXPORT EXIF_API_CALL /**< API export and call macro*/

#ifdef __cplusplus
extern "C" {
#endif

 /**
  * setVerbose()
  *
  * Verbose output on/off
  *
  * parameters
  *  [in] v : 1=on  0=off
  */
void EXIF_API_EXPORT_CALL exif_setVerbose(int);

#ifdef _DEBUG
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
//#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
#include <stdint.h>

    /**
     *   Typical Usage:
     *
     *   The following code describes how to get "Model" Tag's data
     *   from 0th IFD in "test.jpg".
     *
     *   // Parse the JPEG header and create the pointer array of the IFD tables
     *   int result;
     *   void **ifdArray = createIfdTableArray("test.jpg", *result);
     *   switch (result) {
     *     case 0:
     *        :
     *   }
     *   if (!ifdArray) {
     *      return;
     *   }
     *
     *   // Get the TagNodeInfo that matches the IFD_TYPE & TagId
     *   TagNodeInfo *tag = getTagInfo(ifdArray, IFD_0TH, TAG_Model);
     *
     *   if (tag) {
     *      if (!tag->error) {
     *          printf("Model=[%s]\n", tag->byteData); // TYPE_ASCII
     *      }
     *      freeTagInfo(tag);
     *   }
     *
     *   freeIfdTableArray(ifdArray);
     *   return;
     *
     */

     // IFD Type
    typedef enum {
        IFD_UNKNOWN = 0,
        IFD_0TH,
        IFD_1ST,
        IFD_EXIF,
        IFD_GPS,
        IFD_IO,
        IFD_MPF
    } EXIF_IFD_TYPE;

    // Tag Type
    typedef enum {
        TYPE_BYTE = 1,
        TYPE_ASCII,
        TYPE_SHORT,
        TYPE_LONG,
        TYPE_RATIONAL,
        TYPE_SBYTE,
        TYPE_UNDEFINED,
        TYPE_SSHORT,
        TYPE_SLONG,
        TYPE_SRATIONAL
    } EXIF_IFD_TAG_TYPE;

    // Tag info structure
    typedef struct _exif_tagNodeInfo ExifTagNodeInfo;
    struct _exif_tagNodeInfo {
        uint16_t tagId;    // tag ID (e.g. TAG_Model = 0x0110)
        uint16_t type;     // data Type (e.g. TYPE_ASCII = 2)
        unsigned int count;      // count of the data
        unsigned int* numData;   // numeric data array
        uint8_t* byteData; // byte data array
        uint16_t error;    // 0: no error 1: parse error
    };

    typedef struct _exif_image_dir_ent
    {
        uint32_t ImageFlags;
        uint32_t ImageLength;
        uint32_t ImageStart;
        uint16_t Image1EntryNum;
        uint16_t Image2EntryNum;
    } EXIF_IMAGE_DIR_ENT;
    /**
     * Note:
     *
     * [Type = TYPE_BYTE, TYPE_SHORT, TYPE_LONG,
     *         TYPE_SBYTE, TYPE_SSHORT, TYPE_SLONG]
     *  'numData' holds numeric values.
     *  numData[0] to numData[count-1] is accessible.
     *
     * [Type = TYPE_RATIONAL, TYPE_SRATIONAL]
     *  'numData' holds numeric values.
     *  numData[0] to numData[count*2-1] is accessible.
     *
     * [Type = TYPE_ASCII]
     *  'byteData' holds ascii string data.
     *  'count' means the whole length of string with '\0' terminator.
     *
     * [Type = TYPE_UNDEFINED]
     *  'byteData' holds byte data.
     *  byteData[0] to byteData[count-1] is accessible.
     *
     * If the original tag field holds the wrong value, the 'error' flag
     * will be set to 1. In such cases, both 'numData' and 'byteData'
     * might have set to NULL. So, the flag should be checked first.
     */

     // error status
#define EXIF_ERR_READ_FILE            -1
#define EXIF_ERR_WRITE_FILE           -2
#define EXIF_ERR_INVALID_JPEG         -3
#define EXIF_ERR_INVALID_APP1HEADER   -4
#define EXIF_ERR_INVALID_IFD          -5
#define EXIF_ERR_INVALID_ID           -6
#define EXIF_ERR_INVALID_TYPE         -7
#define EXIF_ERR_INVALID_COUNT        -8
#define EXIF_ERR_INVALID_POINTER      -9
#define EXIF_ERR_NOT_EXIST           -10
#define EXIF_ERR_ALREADY_EXIST       -11
#define EXIF_ERR_UNKNOWN             -12
#define EXIF_ERR_MEMALLOC            -13

// public funtions

    /**
     * removeExifSegmentFromJPEGFile()
     *
     * Remove the Exif segment from a JPEG file
     *
     * parameters
     *  [in] inJPEGFileName : original JPEG file
     *  [in] outJPGEFileName : output JPEG file
     *
     * return
     *   1: OK
     *   0: the Exif segment is not found
     *  -n: error
     *      ERR_READ_FILE
     *      ERR_WRITE_FILE
     *      ERR_INVALID_JPEG
     *      ERR_INVALID_APP1HEADER
     */
    int EXIF_API_EXPORT_CALL exif_removeExifSegmentFromJPEGFile(const char* inJPEGFileName,
        const char* outJPGEFileName);

    /**
     * fillIfdTableArray()
     *
     * Parse the JPEG header and fill in the IFD table
     *
     * parameters
     *  [in] JPEGFileName : target JPEG file
     *  [out] ifdArray[32] : array of IfdTable pointers
     *
     * return
     *   n: number of IFD tables
     *   0: the Exif segment is not found
     *  -n: error
     *      ERR_READ_FILE
     *      ERR_INVALID_JPEG
     *      ERR_INVALID_APP1HEADER
     *      ERR_INVALID_IFD
     */
    int EXIF_API_EXPORT_CALL exif_fillIfdTableArray(const char* JPEGFileName, void* ifdArray[32]);

    /**
     * createIfdTableArray()
     *
     * Parse the JPEG header and create the pointer array of the IFD tables
     *
     * parameters
     *  [in] JPEGFileName : target JPEG file
     *  [out] result : result status value
     *   n: number of IFD tables
     *   0: the Exif segment is not found
     *  -n: error
     *      ERR_READ_FILE
     *      ERR_INVALID_JPEG
     *      ERR_INVALID_APP1HEADER
     *      ERR_INVALID_IFD
     *
     * return
     *   NULL: error or no Exif segment
     *  !NULL: pointer array of the IFD tables
     */
    void EXIF_API_EXPORT ** EXIF_API_CALL exif_createIfdTableArray(const char* JPEGFileName, int* result);

    /**
     * freeIfdTables()
     *
     * Free the pointer array of the IFD tables
     *
     * parameters
     *  [in] ifdArray : address of the IFD array
     */
    void EXIF_API_EXPORT_CALL exif_freeIfdTables(void* ifdArray[32]);

    /**
     * freeIfdTableArray()
     *
     * Free the pointer array of the IFD tables
     *
     * parameters
     *  [in] ifdArray : address of the IFD array
     */
    void EXIF_API_EXPORT_CALL exif_freeIfdTableArray(void** ifdArray);

    /**
     * getIfdType()
     *
     * Returns the type of the IFD
     *
     * parameters
     *  [in] ifd: target IFD
     *
     * return
     *  IFD TYPE value
     */
    EXIF_IFD_TYPE EXIF_API_EXPORT_CALL exif_getIfdType(void* ifd);

    /**
     * dumpIfdTable()
     *
     * Dump the IFD table
     *
     * parameters
     *  [in] ifd: target IFD
     */
    void EXIF_API_EXPORT_CALL exif_dumpIfdTable(void* ifd);

    /**
     * dumpIfdTableArray()
     *
     * Dump the array of the IFD tables
     *
     * parameters
     *  [in] ifdArray : address of the IFD array
     */
    void EXIF_API_EXPORT_CALL exif_dumpIfdTableArray(void** ifdArray);

    /**
     * getTagInfo()
     *
     * Get the TagNodeInfo that matches the IFD_TYPE & TagId
     *
     * parameters
     *  [in] ifdArray : address of the IFD array
     *  [in] ifdType : target IFD TYPE
     *  [in] tagId : target tag ID
     *
     * return
     *   NULL: tag is not found
     *  !NULL: address of the TagNodeInfo structure
     */
    ExifTagNodeInfo EXIF_API_EXPORT * EXIF_API_CALL exif_getTagInfo(void** ifdArray,
        EXIF_IFD_TYPE ifdType,
        uint16_t tagId);

    /**
     * getTagInfoFromIfd()
     *
     * Get the TagNodeInfo that matches the TagId
     *
     * parameters
     *  [in] ifd : target IFD
     *  [in] tagId : target tag ID
     *
     * return
     *  NULL: tag is not found
     *  !NULL: address of the TagNodeInfo structure
     */
    ExifTagNodeInfo EXIF_API_EXPORT * EXIF_API_CALL exif_getTagInfoFromIfd(void* ifd, uint16_t tagId);

    /**
     * freeTagInfo()
     *
     * Free the TagNodeInfo allocated by getTagInfo() or getTagInfoFromIfd()
     *
     * parameters
     *  [in] tag : target TagNodeInfo
     */
    void EXIF_API_EXPORT_CALL exif_freeTagInfo(void* tag);

    /**
     * queryTagNodeIsExist()
     *
     * Query if the specified tag node is exist in the IFD tables
     *
     * parameters
     *  [in] ifdTableArray: address of the IFD tables array
     *  [in] ifdType : target IFD type
     *  [in] tagId : target tag ID
     *
     * return
     *  0: not exist
     *  1: exist
     */
    int EXIF_API_EXPORT_CALL exif_queryTagNodeIsExist(void** ifdTableArray,
        EXIF_IFD_TYPE ifdType,
        uint16_t tagId);

    /**
     * createTagInfo()
     *
     * Create new TagNodeInfo block
     *
     * parameters
     *  [in] tagId: id of the tag
     *  [in] type: type of the tag
     *  [in] count: data count of the tag
     *  [out] pResult : error status
     *   0: OK
     *  -n: error
     *      ERR_INVALID_TYPE
     *      ERR_INVALID_COUNT
     *      ERR_MEMALLOC
     *
     * return
     *  NULL: error
     * !NULL: address of the newly created TagNodeInfo
     */
    ExifTagNodeInfo EXIF_API_EXPORT * EXIF_API_CALL exif_createTagInfo(uint16_t tagId,
        uint16_t type,
        unsigned int count,
        int* pResult);

    /**
     * removeIfdTableFromIfdTableArray()
     *
     * Remove the IFD table from the ifdTableArray
     *
     * parameters
     *  [in] ifdTableArray: address of the IFD tables array
     *  [in] ifdType : target IFD type
     *
     * return
     *  n: number of the removed IFD tables
     */
    int EXIF_API_EXPORT_CALL exif_removeIfdTableFromIfdTableArray(void** ifdTableArray, EXIF_IFD_TYPE ifdType);

    /**
     * insertIfdTableToIfdTableArray()
     *
     * Insert new IFD table to the ifdTableArray
     *
     * parameters
     *  [in] ifdTableArray: address of the IFD tables array
     *  [in] ifdType : target IFD type
     *  [out] pResult : result status
     *   0: OK
     *  -n: error
     *      ERR_INVALID_POINTER
     *      ERR_ALREADY_EXIST
     *      ERR_MEMALLOC
     *
     * return
     *  NULL: error
     * !NULL: address of the newly created ifdTableArray
     *
     * note
     * This function frees old ifdTableArray if is not NULL.
     */
    void EXIF_API_EXPORT ** EXIF_API_CALL exif_insertIfdTableToIfdTableArray(void** ifdTableArray,
        EXIF_IFD_TYPE ifdType,
        int* pResult);

    /**
     * removeTagNodeFromIfdTableArray()
     *
     * Remove the specified tag node from the IFD table
     *
     * parameters
     *  [in] ifdTableArray: address of the IFD tables array
     *  [in] ifdType : target IFD type
     *  [in] tagId : target tag ID
     *
     * return
     *  n: number of the removed tags
     */
    int EXIF_API_EXPORT_CALL exif_removeTagNodeFromIfdTableArray(void** ifdTableArray,
        EXIF_IFD_TYPE ifdType,
        uint16_t tagId);

    /**
     * insertTagNodeToIfdTableArray()
     *
     * Insert the specified tag node to the IFD table
     *
     * parameters
     *  [in] ifdTableArray: address of the IFD tables array
     *  [in] ifdType : target IFD type
     *  [in] tagNodeInfo: address of the TagNodeInfo
     *
     * note
     * This function uses the copy of the specified tag data.
     * The caller must free it after this function returns.
     *
     * return
     *  0: OK
     *  ERR_INVALID_POINTER:
     *  ERR_NOT_EXIST:
     *  ERR_ALREADY_EXIST:
     *  ERR_UNKNOWN:
     */
    int EXIF_API_EXPORT_CALL exif_insertTagNodeToIfdTableArray(void** ifdTableArray,
        EXIF_IFD_TYPE ifdType,
        ExifTagNodeInfo* tagNodeInfo);

    /**
     * getThumbnailDataOnIfdTableArray()
     *
     * Get a copy of the thumbnail data from the 1st IFD table
     *
     * parameters
     *  [in] ifdTableArray : address of the IFD tables array
     *  [out] pLength : returns the length of the thumbnail data
     *  [out] pResult : result status
     *   0: OK
     *  -n: error
     *      ERR_INVALID_POINTER
     *      ERR_MEMALLOC
     *      ERR_NOT_EXIST
     *
     * return
     *  NULL: error
     * !NULL: the thumbnail data
     *
     * note
     * This function returns the copy of the thumbnail data.
     * The caller must free it.
     */
    uint8_t EXIF_API_EXPORT * EXIF_API_CALL exif_getThumbnailDataOnIfdTableArray(void** ifdTableArray,
        unsigned int* pLength,
        int* pResult);

    /**
     * setThumbnailDataOnIfdTableArray()
     *
     * Set or update the thumbnail data to the 1st IFD table
     *
     * parameters
     *  [in] ifdTableArray : address of the IFD tables array
     *  [in] pData : thumbnail data
     *  [in] length : thumbnail data length
     *
     * note
     * This function creates the copy of the specified data.
     * The caller must free it after this function returns.
     *
     * return
     *   0: OK
     *  -n: error
     *      ERR_INVALID_POINTER
     *      ERR_MEMALLOC
     *      ERR_UNKNOWN
     */
    int EXIF_API_EXPORT_CALL exif_setThumbnailDataOnIfdTableArray(void** ifdTableArray,
        uint8_t* pData,
        unsigned int length);

    void EXIF_API_EXPORT_CALL exif_getIfdTableDump(void* pIfd, char** pp);


    /**
     * updateExifSegmentInJPEGFile()
     *
     * Update the Exif segment in a JPEG file
     *
     * parameters
     *  [in] inJPEGFileName : original JPEG file
     *  [in] outJPGEFileName : output JPEG file
     *  [in] ifdTableArray : address of the IFD tables array
     *
     * return
     *   1: OK
     *  -n: error
     *      ERR_READ_FILE
     *      ERR_WRITE_FILE
     *      ERR_INVALID_JPEG
     *      ERR_INVALID_APP1HEADER
     *      ERR_INVALID_POINTER
     *      ERROR_UNKNOWN:
     */
    int EXIF_API_EXPORT_CALL exif_updateExifSegmentInJPEGFile(const char* inJPEGFileName,
        const char* outJPGEFileName,
        void** ifdTableArray);

    void EXIF_API_EXPORT_CALL exif_getIfdTableDump(void* pIfd, char** pp);

    /**
     * removeAdobeMetadataSegmentFromJPEGFile()
     *
     * Remove Adobe's XMP metadata segment from a JPEG file
     *
     * parameters
     *  [in] inJPEGFileName : original JPEG file
     *  [in] outJPGEFileName : output JPEG file
     *
     * return
     *   1: OK
     *   0: Adobe's metadata segment is not found
     *  -n: error
     *      ERR_READ_FILE
     *      ERR_WRITE_FILE
     *      ERR_INVALID_JPEG
     *      ERR_INVALID_APP1HEADER
     */
    int EXIF_API_EXPORT_CALL exif_removeAdobeMetadataSegmentFromJPEGFile(const char* inJPEGFileName,
        const char* outJPGEFileName);

    // Tag IDs
    // 0th IFD, 1st IFD, Exif IFD
#define TAG_ImageWidth                   0x0100
#define TAG_ImageLength                  0x0101
#define TAG_BitsPerSample                0x0102
#define TAG_Compression                  0x0103
#define TAG_PhotometricInterpretation    0x0106
#define TAG_Orientation                  0x0112
#define TAG_SamplesPerPixel              0x0115
#define TAG_PlanarConfiguration          0x011C
#define TAG_YCbCrSubSampling             0x0212
#define TAG_YCbCrPositioning             0x0213
#define TAG_XResolution                  0x011A
#define TAG_YResolution                  0x011B
#define TAG_ResolutionUnit               0x0128

#define TAG_StripOffsets                 0x0111
#define TAG_RowsPerStrip                 0x0116
#define TAG_StripByteCounts              0x0117
#define TAG_JPEGInterchangeFormat        0x0201
#define TAG_JPEGInterchangeFormatLength  0x0202

#define TAG_TransferFunction             0x012D
#define TAG_WhitePoint                   0x013E
#define TAG_PrimaryChromaticities        0x013F
#define TAG_YCbCrCoefficients            0x0211
#define TAG_ReferenceBlackWhite          0x0214

#define TAG_DateTime                     0x0132
#define TAG_ImageDescription             0x010E
#define TAG_Make                         0x010F
#define TAG_Model                        0x0110
#define TAG_Software                     0x0131
#define TAG_Artist                       0x013B
#define TAG_Copyright                    0x8298
#define TAG_ExifIFDPointer               0x8769
#define TAG_GPSInfoIFDPointer            0x8825
#define TAG_InteroperabilityIFDPointer   0xA005

#define TAG_Rating                       0x4746

#define TAG_ExifVersion                  0x9000
#define TAG_FlashPixVersion              0xA000

#define TAG_ColorSpace                   0xA001

#define TAG_ComponentsConfiguration      0x9101
#define TAG_CompressedBitsPerPixel       0x9102
#define TAG_PixelXDimension              0xA002
#define TAG_PixelYDimension              0xA003

#define TAG_MakerNote                    0x927C
#define TAG_UserComment                  0x9286

#define TAG_RelatedSoundFile             0xA004

#define TAG_DateTimeOriginal             0x9003
#define TAG_DateTimeDigitized            0x9004
#define TAG_SubSecTime                   0x9290
#define TAG_SubSecTimeOriginal           0x9291
#define TAG_SubSecTimeDigitized          0x9292

#define TAG_ExposureTime                 0x829A
#define TAG_FNumber                      0x829D
#define TAG_ExposureProgram              0x8822
#define TAG_SpectralSensitivity          0x8824
#define TAG_PhotographicSensitivity      0x8827
#define TAG_OECF                         0x8828
#define TAG_SensitivityType              0x8830
#define TAG_StandardOutputSensitivity    0x8831
#define TAG_RecommendedExposureIndex     0x8832
#define TAG_ISOSpeed                     0x8833
#define TAG_ISOSpeedLatitudeyyy          0x8834
#define TAG_ISOSpeedLatitudezzz          0x8835

#define TAG_ShutterSpeedValue            0x9201
#define TAG_ApertureValue                0x9202
#define TAG_BrightnessValue              0x9203
#define TAG_ExposureBiasValue            0x9204
#define TAG_MaxApertureValue             0x9205
#define TAG_SubjectDistance              0x9206
#define TAG_MeteringMode                 0x9207
#define TAG_LightSource                  0x9208
#define TAG_Flash                        0x9209
#define TAG_FocalLength                  0x920A
#define TAG_SubjectArea                  0x9214
#define TAG_FlashEnergy                  0xA20B
#define TAG_SpatialFrequencyResponse     0xA20C
#define TAG_FocalPlaneXResolution        0xA20E
#define TAG_FocalPlaneYResolution        0xA20F
#define TAG_FocalPlaneResolutionUnit     0xA210
#define TAG_SubjectLocation              0xA214
#define TAG_ExposureIndex                0xA215
#define TAG_SensingMethod                0xA217
#define TAG_FileSource                   0xA300
#define TAG_SceneType                    0xA301
#define TAG_CFAPattern                   0xA302

#define TAG_CustomRendered               0xA401
#define TAG_ExposureMode                 0xA402
#define TAG_WhiteBalance                 0xA403
#define TAG_DigitalZoomRatio             0xA404
#define TAG_FocalLengthIn35mmFormat      0xA405
#define TAG_SceneCaptureType             0xA406
#define TAG_GainControl                  0xA407
#define TAG_Contrast                     0xA408
#define TAG_Saturation                   0xA409
#define TAG_Sharpness                    0xA40A
#define TAG_DeviceSettingDescription     0xA40B
#define TAG_SubjectDistanceRange         0xA40C

#define TAG_ImageUniqueID                0xA420
#define TAG_CameraOwnerName              0xA430
#define TAG_BodySerialNumber             0xA431
#define TAG_LensSpecification            0xA432
#define TAG_LensMake                     0xA433
#define TAG_LensModel                    0xA434
#define TAG_LensSerialNumber             0xA435
#define TAG_Gamma                        0xA500

#define TAG_PrintIM                      0xC4A5
#define TAG_Padding                      0xEA1C

// GPS IFD
#define TAG_GPSVersionID                 0x0000
#define TAG_GPSLatitudeRef               0x0001
#define TAG_GPSLatitude                  0x0002
#define TAG_GPSLongitudeRef              0x0003
#define TAG_GPSLongitude                 0x0004
#define TAG_GPSAltitudeRef               0x0005
#define TAG_GPSAltitude                  0x0006
#define TAG_GPSTimeStamp                 0x0007
#define TAG_GPSSatellites                0x0008
#define TAG_GPSStatus                    0x0009
#define TAG_GPSMeasureMode               0x000A
#define TAG_GPSDOP                       0x000B
#define TAG_GPSSpeedRef                  0x000C
#define TAG_GPSSpeed                     0x000D
#define TAG_GPSTrackRef                  0x000E
#define TAG_GPSTrack                     0x000F
#define TAG_GPSImgDirectionRef           0x0010
#define TAG_GPSImgDirection              0x0011
#define TAG_GPSMapDatum                  0x0012
#define TAG_GPSDestLatitudeRef           0x0013
#define TAG_GPSDestLatitude              0x0014
#define TAG_GPSDestLongitudeRef          0x0015
#define TAG_GPSDestLongitude             0x0016
#define TAG_GPSBearingRef                0x0017
#define TAG_GPSBearing                   0x0018
#define TAG_GPSDestDistanceRef           0x0019
#define TAG_GPSDestDistance              0x001A
#define TAG_GPSProcessingMethod          0x001B
#define TAG_GPSAreaInformation           0x001C
#define TAG_GPSDateStamp                 0x001D
#define TAG_GPSDifferential              0x001E
#define TAG_GPSHPositioningError         0x001F

// Interoperability IFD
#define TAG_InteroperabilityIndex        0x0001
#define TAG_InteroperabilityVersion      0x0002

#define TAG_RelatedImageFileFormat       0x1000
#define TAG_RelatedImageWidth            0x1001
#define TAG_RelatedImageHeight           0x1002

// MPF tags
#define TAG_MPFVersion                   0xB000
#define TAG_NumberOfImage                0xB001
#define TAG_MPImageList                  0xB002
#define TAG_ImageUIDList                 0xB003
#define TAG_TotalFrames                  0xB004

#define TAG_MPIndividualNum              0xB101

#define TAG_PanOrientation               0xB201
#define TAG_PanOverlapH                  0xB202
#define TAG_PanOverlapV                  0xB203
#define TAG_BaseViewpointNum             0xB204
#define TAG_ConvergenceAngle             0xB205
#define TAG_BaselineLength               0xB206
#define TAG_VerticalDivergence           0xB207
#define TAG_AxisDistanceX                0xB208
#define TAG_AxisDistanceY                0xB209
#define TAG_AxisDistanceZ                0xB20A
#define TAG_YawAngle                     0xB20B
#define TAG_PitchAngle                   0xB20C
#define TAG_RollAngle                    0xB20D


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // _EXIF_H_
