/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/devernay/openfx-supportext>,
 * Copyright (C) 2015 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * OFX Resolution helper
 */

#ifndef IO_ofxsResolution_h
#define IO_ofxsResolution_h

#define kParamFormatPCVideo        "PC-Video"
#define kParamFormatPCVideoLabel   "PC-Video            640x480"
#define kParamFormatNTSC           "NTSC"
#define kParamFormatNTSCLabel      "NTSC                720x486 0.91"
#define kParamFormatPAL            "PAL"
#define kParamFormatPALLabel       "PAL                 720x576 1.09"
#define kParamFormatHD             "HD"
#define kParamFormatHDLabel        "HD                  1920x1080"
#define kParamFormatNTSC169        "NTSC-16:9"
#define kParamFormatNTSC169Label   "NTSC-16:9           720x486 1.21"
#define kParamFormatPAL169         "PAL-16:9"
#define kParamFormatPAL169Label    "PAL-16:9            720x576 1.46"
#define kParamFormat1kSuper35      "1K-Super35-full-ap"
#define kParamFormat1kSuper35Label "1K-Super35-full-ap  1024x778"
#define kParamFormat1kCinemascope  "1K-Cinemascope"
#define kParamFormat1kCinemascopeLabel "1K-Cinemascope      914x778 2"
#define kParamFormat2kSuper35      "2K-Super35-full-ap"
#define kParamFormat2kSuper35Label "2K-Super35-full-ap  2048x1556"
#define kParamFormat2kCinemascope  "2K-Cinemascope"
#define kParamFormat2kCinemascopeLabel "2K-Cinemascope      1828x1556 2"
#define kParamFormat4kSuper35      "4K-Super35-full-ap"
#define kParamFormat4kSuper35Label "4K-Super35-full-ap  4096x3112"
#define kParamFormat4kCinemascope  "4K-Cinemascope"
#define kParamFormat4kCinemascopeLabel  "4K-Cinemascope      3656x3112 2"
#define kParamFormatSquare256      "Square-256"
#define kParamFormatSquare256Label "Square-256          256x256"
#define kParamFormatSquare512      "Square-512"
#define kParamFormatSquare512Label "Square-512          512x512"
#define kParamFormatSquare1k       "Square-1k"
#define kParamFormatSquare1kLabel  "Square-1k           1024x1024"
#define kParamFormatSquare2k       "Square-2k"
#define kParamFormatSquare2kLabel  "Square-2k           2048x2048"

namespace OFX {
enum EParamFormat
{
    eParamFormatPCVideo,
    eParamFormatNTSC,
    eParamFormatPAL,
    eParamFormatHD,
    eParamFormatNTSC169,
    eParamFormatPAL169,
    eParamFormat1kSuper35,
    eParamFormat1kCinemascope,
    eParamFormat2kSuper35,
    eParamFormat2kCinemascope,
    eParamFormat4kSuper35,
    eParamFormat4kCinemascope,
    eParamFormatSquare256,
    eParamFormatSquare512,
    eParamFormatSquare1k,
    eParamFormatSquare2k,
    eParamFormatCount
};

inline void
getFormatResolution(const EParamFormat format,
                    int *width,
                    int *height,
                    double *par)
{
    switch (format) {
    case eParamFormatPCVideo:
        *width =  640; *height =  480; *par = 1.; break;
    case eParamFormatNTSC:
        *width =  720; *height =  486; *par = 0.91; break;
    case eParamFormatPAL:
        *width =  720; *height =  576; *par = 1.09; break;
    case eParamFormatHD:
        *width = 1920; *height = 1080; *par = 1.; break;
    case eParamFormatNTSC169:
        *width =  720; *height =  486; *par = 1.21; break;
    case eParamFormatPAL169:
        *width =  720; *height =  576; *par = 1.46; break;
    case eParamFormat1kSuper35:
        *width = 1024; *height =  778; *par = 1.; break;
    case eParamFormat1kCinemascope:
        *width =  914; *height =  778; *par = 2.; break;
    case eParamFormat2kSuper35:
        *width = 2048; *height = 1556; *par = 1.; break;
    case eParamFormat2kCinemascope:
        *width = 1828; *height = 1556; *par = 2.; break;
    case eParamFormat4kSuper35:
        *width = 4096; *height = 3112; *par = 1.; break;
    case eParamFormat4kCinemascope:
        *width = 3656; *height = 3112; *par = 2.; break;
    case eParamFormatSquare256:
        *width =  256; *height =  256; *par = 1.; break;
    case eParamFormatSquare512:
        *width =  512; *height =  512; *par = 1.; break;
    case eParamFormatSquare1k:
        *width = 1024; *height = 1024; *par = 1.; break;
    case eParamFormatSquare2k:
        *width = 2048; *height = 2048; *par = 1.; break;
    default:
        break;
    }
}
}

#endif // ifndef IO_ofxsResolution_h

