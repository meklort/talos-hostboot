/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/common/utils/imageProcs/common_ringId.C $    */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2017,2018                        */
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
#include <common_ringId.H>

namespace P9_RID
{
#include <p9_ringId.H>
};
namespace CEN_RID
{
#include <cen_ringId.H>
};
#include <p9_infrastruct_help.H>

// These strings must adhere precisely to the enum of PpeType.
const char* ppeTypeName[] = { "SBE",
                              "CME",
                              "SGPE"
                            };

// These strings must adhere precisely to the enum of RingVariant.
const char* ringVariantName[] = { "BASE",
                                  "CC",
                                  "RL",
                                  "RL2",
                                  "RL3",
                                  "RL4",
                                  "RL5"
                                };

#ifndef __HOSTBOOT_MODULE  // This is only used by ring_apply in EKB

static int get_ipl_ring_path_param( RingVariant_t i_ringVariant, char*& o_ringPath)
{
    switch (i_ringVariant)
    {
        case RV_BASE:
            o_ringPath = getenv("IPL_BASE");
            break;

        case RV_CC:
            o_ringPath = getenv("IPL_CACHE_CONTAINED");
            break;

        case RV_RL:
            o_ringPath = getenv("IPL_RISK");
            break;

        default:
            o_ringPath = NULL;
            break;
    }

    if (o_ringPath == NULL)
    {
        MY_ERR("get_ipl_ring_path_param(): IPL env parm for ringVariant=0x%x not set.\n",
               i_ringVariant);
        return INFRASTRUCT_RC_ENV_ERROR;
    }

    return INFRASTRUCT_RC_SUCCESS;
}

static int get_runtime_ring_path_param( RingVariant_t i_ringVariant, char*& o_ringPath)
{
    switch (i_ringVariant)
    {
        case RV_BASE:
            o_ringPath = getenv("RUNTIME_BASE");
            break;

        case RV_RL:
            o_ringPath = getenv("RUNTIME_RISK");
            break;

        case RV_RL2:
            o_ringPath = getenv("RUNTIME_RISK2");
            break;

        case RV_RL3:
            o_ringPath = getenv("RUNTIME_RISK3");
            break;

        case RV_RL4:
            o_ringPath = getenv("RUNTIME_RISK4");
            break;

        case RV_RL5:
            o_ringPath = getenv("RUNTIME_RISK5");
            break;

        default:
            o_ringPath = NULL;
            break;
    }

    if (o_ringPath == NULL)
    {
        MY_ERR("get_runtime_ring_path_param(): RUNTIME env parm for ringVariant=0x%x not set.\n",
               i_ringVariant);
        return INFRASTRUCT_RC_ENV_ERROR;
    }

    return INFRASTRUCT_RC_SUCCESS;
}

int ringid_get_raw_ring_file_path( uint32_t        i_magic,
                                   RingVariant_t   i_ringVariant,
                                   char*           io_ringPath )
{
    int   rc = INFRASTRUCT_RC_SUCCESS;
    char* l_ringDir = NULL;

    do
    {

        if ( i_magic == TOR_MAGIC_SBE )
        {
            if ( i_ringVariant == RV_BASE ||
                 i_ringVariant == RV_CC ||
                 i_ringVariant == RV_RL )
            {
                rc = get_ipl_ring_path_param(i_ringVariant, l_ringDir);
            }
            else if ( i_ringVariant == RV_RL2 ||
                      i_ringVariant == RV_RL3 ||
                      i_ringVariant == RV_RL4 ||
                      i_ringVariant == RV_RL5 )
            {
                // No IPL rings for these variants
                rc = TOR_NO_RINGS_FOR_VARIANT;
            }
            else
            {
                MY_ERR("Invalid ringVariant(=%d) for TOR magic=0x%08x\n",
                       i_ringVariant, i_magic);
                rc = TOR_INVALID_VARIANT;
            }

            if (rc)
            {
                break;
            }

            strcat(io_ringPath, l_ringDir);
            strcat(io_ringPath, "/");
        }
        else if ( i_magic == TOR_MAGIC_CME ||
                  i_magic == TOR_MAGIC_SGPE )
        {
            if ( i_ringVariant == RV_BASE ||
                 i_ringVariant == RV_RL ||
                 i_ringVariant == RV_RL2 ||
                 i_ringVariant == RV_RL3 ||
                 i_ringVariant == RV_RL4 ||
                 i_ringVariant == RV_RL5 )
            {
                rc = get_runtime_ring_path_param(i_ringVariant, l_ringDir);
            }
            else if ( i_ringVariant == RV_CC )
            {
                // No Runtime rings for this variant
                rc = TOR_NO_RINGS_FOR_VARIANT;
            }
            else
            {
                MY_ERR("Invalid ringVariant(=%d) for TOR magic=0x%08x\n",
                       i_ringVariant, i_magic);
                rc = TOR_INVALID_VARIANT;
            }

            if (rc)
            {
                break;
            }

            strcat(io_ringPath, l_ringDir);
            strcat(io_ringPath, "/");
        }
        else if ( i_magic == TOR_MAGIC_CEN )
        {
            if ( i_ringVariant == RV_BASE ||
                 i_ringVariant == RV_RL )
            {
                rc = get_ipl_ring_path_param(i_ringVariant, l_ringDir);
            }
            else
            {
                MY_ERR("Invalid ringVariant(=%d) for TOR magic=0x%08x\n",
                       i_ringVariant, i_magic);
                rc = TOR_INVALID_VARIANT;
            }

            if (rc)
            {
                break;
            }

            strcat(io_ringPath, l_ringDir);
            strcat(io_ringPath, "/");
        }
        else if ( i_magic == TOR_MAGIC_OVRD ||
                  i_magic == TOR_MAGIC_OVLY )
        {
            // Path already fully qualified. Return io_ringPath as is.
        }
        else
        {
            MY_ERR("Unsupported value of TOR magic(=0x%X)\n", i_magic);
            rc = TOR_INVALID_MAGIC_NUMBER;
        }

    }
    while(0);

    return rc;
}

#endif // End of  ifndef __HOSTBOOT_MODULE


int ringid_get_noof_ring_ids( ChipType_t  i_chipType,
                              RingId_t*   o_numRingIds)
{
    int rc = INFRASTRUCT_RC_SUCCESS;

    switch (i_chipType)
    {
        case CT_P9N:
        case CT_P9C:
        case CT_P9A:
            *o_numRingIds = P9_RID::NUM_RING_IDS;
            break;

        case CT_CEN:
            *o_numRingIds = CEN_RID::NUM_RING_IDS;
            break;

        default:
            MY_ERR("ringid_get_noof_ring_ids(): Unsupported chipType (=%d) supplied",
                   i_chipType);
            rc = TOR_INVALID_CHIPTYPE;
            break;
    }

    return rc;
}


int ringid_get_ring_list( ChipType_t       i_chipType,
                          RingId_t         i_ringId,
                          GenRingIdList**  o_ringIdList)
{
    int rc = INFRASTRUCT_RC_SUCCESS;

    switch (i_chipType)
    {
        case CT_P9N:
        case CT_P9C:
        case CT_P9A:
            *o_ringIdList = P9_RID::_ringid_get_ring_list(i_ringId);
            break;

        case CT_CEN:
            *o_ringIdList = CEN_RID::_ringid_get_ring_list(i_ringId);
            break;

        default:
            MY_ERR("ringid_get_ring_list(): Unsupported chipType (=%d) supplied",
                   i_chipType);
            rc = TOR_INVALID_CHIPTYPE;
            break;
    }

    return rc;
}


int ringid_get_noof_chiplets( ChipType_t  i_chipType,
                              uint32_t    i_torMagic,
                              uint8_t*    o_numChiplets )
{
    switch (i_chipType)
    {
        case CT_P9N:
        case CT_P9C:
        case CT_P9A:
            if ( i_torMagic == TOR_MAGIC_SBE  ||
                 i_torMagic == TOR_MAGIC_OVRD ||
                 i_torMagic == TOR_MAGIC_OVLY )
            {
                *o_numChiplets = P9_RID::SBE_NOOF_CHIPLETS;
            }
            else if ( i_torMagic == TOR_MAGIC_CME )
            {
                *o_numChiplets = P9_RID::CME_NOOF_CHIPLETS;
            }
            else if ( i_torMagic == TOR_MAGIC_SGPE )
            {
                *o_numChiplets = P9_RID::SGPE_NOOF_CHIPLETS;
            }
            else
            {
                MY_ERR("Invalid torMagic (=0x%08x) for chipType (=CT_P9x=%d)\n", i_torMagic, i_chipType);
                return TOR_INVALID_MAGIC_NUMBER;
            }

            break;

        case CT_CEN:
            if ( i_torMagic == TOR_MAGIC_CEN ||
                 i_torMagic == TOR_MAGIC_OVRD )
            {
                *o_numChiplets = CEN_RID::CEN_NOOF_CHIPLETS;
            }
            else
            {
                MY_ERR("Invalid torMagic (=0x%08x) for chipType (=CT_CEN)\n", i_torMagic);
                return TOR_INVALID_MAGIC_NUMBER;
            }

            break;

        default:
            MY_ERR("Invalid chipType (=0x%02x)\n", i_chipType);
            return TOR_INVALID_CHIPTYPE;
    }

    return TOR_SUCCESS;
}


int  ringid_get_properties( ChipType_t         i_chipType,
                            uint32_t           i_torMagic,
                            uint8_t            i_torVersion,
                            ChipletType_t      i_chipletType,
                            ChipletData_t**    o_chipletData,
                            GenRingIdList**    o_ringIdListCommon,
                            GenRingIdList**    o_ringIdListInstance,
                            RingVariantOrder** o_ringVariantOrder,
                            RingProperties_t** o_ringProps,
                            uint8_t*           o_numVariants )
{
    switch (i_chipType)
    {
        case CT_P9N:
        case CT_P9C:
        case CT_P9A:
            if ( i_torMagic == TOR_MAGIC_SBE  ||
                 i_torMagic == TOR_MAGIC_OVRD ||
                 i_torMagic == TOR_MAGIC_OVLY )
            {
                P9_RID::ringid_get_chiplet_properties(
                    i_chipletType,
                    o_chipletData,
                    o_ringIdListCommon,
                    o_ringIdListInstance,
                    o_ringVariantOrder,
                    o_numVariants);

                if ( i_torVersion < 7 &&
                     (i_chipletType == P9_RID::EQ_TYPE || i_chipletType == P9_RID::EC_TYPE) )
                {
                    *o_numVariants = *o_numVariants - 3;
                }

                if ( i_torMagic == TOR_MAGIC_OVRD ||
                     i_torMagic == TOR_MAGIC_OVLY )
                {
                    *o_numVariants = 1;
                }
            }
            else if ( i_torMagic == TOR_MAGIC_CME )
            {
                *o_chipletData        = (ChipletData_t*)&P9_RID::EC::g_chipletData;
                *o_ringIdListCommon   = (GenRingIdList*)P9_RID::EC::RING_ID_LIST_COMMON;
                *o_ringIdListInstance = (GenRingIdList*)P9_RID::EC::RING_ID_LIST_INSTANCE;
                *o_ringVariantOrder   = (RingVariantOrder*)P9_RID::EC::RING_VARIANT_ORDER;
                *o_numVariants        = P9_RID::EC::g_chipletData.iv_num_common_ring_variants;

                if (i_torVersion < 7)
                {
                    *o_numVariants = *o_numVariants - 3;
                }
            }
            else if ( i_torMagic == TOR_MAGIC_SGPE )
            {
                *o_chipletData        = (ChipletData_t*)&P9_RID::EQ::g_chipletData;
                *o_ringIdListCommon   = (GenRingIdList*)P9_RID::EQ::RING_ID_LIST_COMMON;
                *o_ringIdListInstance = (GenRingIdList*)P9_RID::EQ::RING_ID_LIST_INSTANCE;
                *o_ringVariantOrder   = (RingVariantOrder*)P9_RID::EQ::RING_VARIANT_ORDER;
                *o_numVariants        = P9_RID::EQ::g_chipletData.iv_num_common_ring_variants;

                if (i_torVersion < 7)
                {
                    *o_numVariants = *o_numVariants - 3;
                }
            }
            else
            {
                MY_ERR("Invalid torMagic (=0x%08x) for chipType=CT_P9x=%d\n", i_torMagic, i_chipType);
                return TOR_INVALID_MAGIC_NUMBER;
            }

            *o_ringProps = (RingProperties_t*)P9_RID::RING_PROPERTIES;

            break;

        case CT_CEN:
            if ( i_torMagic == TOR_MAGIC_CEN ||
                 i_torMagic == TOR_MAGIC_OVRD )
            {
                CEN_RID::ringid_get_chiplet_properties(
                    i_chipletType,
                    o_chipletData,
                    o_ringIdListCommon,
                    o_ringIdListInstance,
                    o_ringVariantOrder,
                    o_numVariants);

                if ( i_torMagic == TOR_MAGIC_OVRD)
                {
                    *o_numVariants = 1;
                }
            }
            else
            {
                MY_ERR("Invalid torMagic (=0x%08x) for chipType=CT_CEN\n", i_torMagic);
                return TOR_INVALID_MAGIC_NUMBER;
            }

            *o_ringProps = (RingProperties_t*)CEN_RID::RING_PROPERTIES;

            break;

        default:
            MY_ERR("Invalid chipType (=0x%02x)\n", i_chipType);
            return TOR_INVALID_CHIPTYPE;

    }

    return TOR_SUCCESS;
}
