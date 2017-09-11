/*
  Copyright (c) 2013, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.


   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
*/

#include <cstdint>

struct rgba_surface 
{
    uint8_t* ptr;
    int32_t width;
    int32_t height;
    int32_t stride;
};

struct bc7_enc_settings
{
    bool mode_selection[4];
    int refineIterations[8];

    bool skip_mode2;
    int fastSkipTreshold_mode1;
    int fastSkipTreshold_mode3;
    int fastSkipTreshold_mode7;

    int mode45_channel0;
    int refineIterations_channel;

    int channels;
};

struct bc6h_enc_settings
{
    bool slow_mode;
    bool fast_mode;
    int refineIterations_1p;
    int refineIterations_2p;
    int fastSkipTreshold;
};

// profiles for RGB data (alpha channel will be ignored)
extern "C" void GetProfile_ultrafast(bc7_enc_settings* settings);
extern "C" void GetProfile_veryfast(bc7_enc_settings* settings);
extern "C" void GetProfile_fast(bc7_enc_settings* settings);
extern "C" void GetProfile_basic(bc7_enc_settings* settings);
extern "C" void GetProfile_slow(bc7_enc_settings* settings);

// profiles for RGBA inputs
extern "C" void GetProfile_alpha_ultrafast(bc7_enc_settings* settings);
extern "C" void GetProfile_alpha_veryfast(bc7_enc_settings* settings);
extern "C" void GetProfile_alpha_fast(bc7_enc_settings* settings);
extern "C" void GetProfile_alpha_basic(bc7_enc_settings* settings);
extern "C" void GetProfile_alpha_slow(bc7_enc_settings* settings);

// profiles for BC6H (RGB HDR)
extern "C" void GetProfile_bc6h_veryfast(bc6h_enc_settings* settings);
extern "C" void GetProfile_bc6h_fast(bc6h_enc_settings* settings);
extern "C" void GetProfile_bc6h_basic(bc6h_enc_settings* settings);
extern "C" void GetProfile_bc6h_slow(bc6h_enc_settings* settings);
extern "C" void GetProfile_bc6h_veryslow(bc6h_enc_settings* settings);

/*
	Notes:
	  - input width and height need to be a multiple of 4
	  - dst buffer must be allocated with enough space for the compressed texture:
		(width/4)*(height/4) blocks, 4 bytes/block for BC1, 8 bytes/block for BC7/BC6H
		the blocks are stored in raster scan order (natural CPU texture layout)
	  - you can use GetProfile_* functions to select various speed/quality tradeoffs.
	  - the RGB profiles are slightly faster as they ignore the alpha channel
*/

extern "C" void CompressBlocksBC1(const rgba_surface* src, uint8_t* dst);
extern "C" void CompressBlocksBC3(const rgba_surface* src, uint8_t* dst);
extern "C" void CompressBlocksBC6H(const rgba_surface* src, uint8_t* dst, bc6h_enc_settings* settings);
extern "C" void CompressBlocksBC7(const rgba_surface* src, uint8_t* dst, bc7_enc_settings* settings);
