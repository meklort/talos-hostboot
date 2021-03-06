/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/scom/plugins/errludP_scom.H $                         */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2016,2017                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#ifndef ERRL_UDP_SCOM_H
#define ERRL_UDP_SCOM_H

/**
*  @file errludP_scom.H
*
*  Defines the ErrlUserDetailsParser classes that parse SCOM FFDC
*/

#include "errluserdetails.H"
#include <string.h>

/**
* Some macros to manipulate data types cleanly
*/
#define TO_UINT8(ptr)   (*(reinterpret_cast<uint8_t*>(ptr)))


namespace SCOM
{

    /**
    * @class UdParserPib
    *
    * Parses UdPibInfo
    */
    class UdParserPib : public ERRORLOG::ErrlUserDetailsParser
    {
        public:
            /**
            *  @brief Constructor
            */
            UdParserPib() {}

            /**
            *  @brief Destructor
            */
            virtual ~UdParserPib() {}

            /**
            *  @brief Parses string user detail data from an error log
            *
            *  @param  i_version Version of the data
            *  @param  i_parse   ErrlUsrParser object for outputting information
            *  @param  i_pBuffer Pointer to buffer containing detail data
            *  @param  i_buflen  Length of the buffer
            */
            virtual void parse(errlver_t i_version,
                               ErrlUsrParser & i_parser,
                               void * i_pBuffer,
                               const uint32_t i_buflen) const
            {
                char* l_databuf = static_cast<char*>(i_pBuffer);
                i_parser.PrintHeading("SCOM PIB ERR");

                //***** Memory Layout *****
                // 1 bytes  : Pib Error

                const char* l_decodeStr = "Unknown";
                uint8_t l_piberr = TO_UINT8(l_databuf);
                switch(l_piberr)
                {
                    case(0): l_decodeStr = "None"; break;
                    case(1): l_decodeStr = "Resource Occupied"; break;
                    case(2): l_decodeStr = "Chiplet Offline"; break;
                    case(3): l_decodeStr = "Partial Good"; break;
                    case(4): l_decodeStr = "Invalid Address"; break;
                    case(5): l_decodeStr = "Clock Error"; break;
                    case(6): l_decodeStr = "Parity Error"; break;
                    case(7): l_decodeStr = "Timeout"; break;
                }
                char l_outputStr[30];
                sprintf( l_outputStr, "%d (%s)", l_piberr, l_decodeStr );
                i_parser.PrintString("Pib Err", l_outputStr );
            }

          private:

            // Parser isn't compiled with c++11 in all environments, and
            // therefore "delete" of unused interfaces (like below) is not
            // supported, nor are functions with move semantics

            // Disable compiler provided default functions
            UdParserPib(
                const UdParserPib&);
            UdParserPib & operator=(
                const UdParserPib&);
    };

}

#endif
