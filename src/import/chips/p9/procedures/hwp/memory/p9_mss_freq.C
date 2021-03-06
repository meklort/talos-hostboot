/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/procedures/hwp/memory/p9_mss_freq.C $     */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2018                        */
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

///
/// @file p9_mss_freq.C
/// @brief Calculate and save off DIMM frequencies
///
// *HWP HWP Owner: Andre Marin <aamarin@us.ibm.com>
// *HWP HWP Backup: Louis Stermole <stermole@us.ibm.com>
// *HWP Team: Memory
// *HWP Level: 3
// *HWP Consumed by: FSP:HB

//----------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------
#include <p9_mss_freq.H>

// std lib
#include <map>
#include <utility>
// fapi2
#include <fapi2.H>

// mss lib
#include <generic/memory/lib/spd/common/ddr4/spd_decoder_ddr4.H>
#include <lib/spd/spd_factory.H>
#include <lib/freq/cas_latency.H>
#include <lib/freq/sync.H>
#include <lib/workarounds/freq_workarounds.H>
#include <generic/memory/lib/utils/c_str.H>
#include <generic/memory/lib/utils/find.H>
#include <generic/memory/lib/utils/count_dimm.H>
#include <generic/memory/lib/utils/index.H>
#include <lib/shared/mss_const.H>

using fapi2::TARGET_TYPE_MCS;
using fapi2::TARGET_TYPE_MCA;
using fapi2::TARGET_TYPE_DIMM;
using fapi2::TARGET_TYPE_MCBIST;
using fapi2::FAPI2_RC_SUCCESS;

extern "C"
{
    ///
    /// @brief Calculate and save off DIMM frequencies
    /// @param[in] i_target, the controller (e.g., MCS)
    /// @return FAPI2_RC_SUCCESS iff ok
    ///
    fapi2::ReturnCode p9_mss_freq( const fapi2::Target<TARGET_TYPE_MCS>& i_target )
    {
        // TODO RTC:161701 p9_mss_freq needs to be re-worked to work per-MC as it's hard
        // (if not impossible) to know the appropriate freq if we don't have information
        // for both MCS. Setting one to max freq doesn't work in the case of 0 DIMM as
        // there is no check for other freq's and we end up setting the chosen freq to
        // the default.
        // So for now, iterate over all the MCBIST. This isn't great as we do this work
        // twice for every MC. However, attribute access is cheap so this will suffice for
        // the time being.
        const auto& l_mcbist = mss::find_target<TARGET_TYPE_MCBIST>(i_target);
        std::vector< std::vector<uint64_t> > l_min_dimm_freq(mss::MCS_PER_MC, std::vector<uint64_t> (mss::PORTS_PER_MCS,  0) );
        std::vector<uint32_t> l_supported_freqs;

        uint64_t l_mss_freq = 0;
        uint32_t l_nest_freq = 0;

        // If there are no DIMM, we can just get out.
        if (mss::count_dimm(l_mcbist) == 0)
        {
            FAPI_INF("Seeing no DIMM on %s, no freq to set", mss::c_str(l_mcbist));
            return FAPI2_RC_SUCCESS;
        }

        // Get supported freqs for this MCBIST
        FAPI_TRY( mss::supported_freqs(l_mcbist, l_supported_freqs) );

        for (const auto& l_mcs : mss::find_targets<TARGET_TYPE_MCS>(l_mcbist))
        {
            const auto l_mcs_index = mss::index(l_mcs);

            std::vector< std::pair< uint64_t, fapi2::Target<fapi2::TARGET_TYPE_MCA>> > l_desired_cas_latency;

            for (const auto& l_mca : mss::find_targets<TARGET_TYPE_MCA>(l_mcs) )
            {
                const auto l_mca_index = mss::index(l_mca);
                std::vector< std::shared_ptr<mss::spd::decoder> > l_factory_caches;
                fapi2::ReturnCode l_rc;

                // Get cached decoder
                FAPI_TRY( mss::spd::populate_decoder_caches(l_mca, l_factory_caches),
                          "%s. Failed to populate decoder cache", mss::c_str(l_mca) );

                // Instantiation of class that calculates CL algorithm
                mss::cas_latency l_cas_latency( l_mca, l_factory_caches, l_supported_freqs, l_rc );

                FAPI_TRY( l_rc, "%s. Failed to initialize cas_latency ctor", mss::c_str(l_mca) );

                if(l_cas_latency.iv_dimm_list_empty)
                {
                    // Cannot fail out for an empty DIMM configuration, so default values are set
                    FAPI_INF("%s. DIMM list is empty! Setting default values for CAS latency and DIMM speed.",
                             mss::c_str(l_mca) );
                }

                else
                {
                    // We set this to a non-0 so we avoid divide-by-zero errors in the conversions which
                    // go from clocks to time (and vice versa.) We have other bugs if there was really
                    // no MT/s determined and there really is a DIMM installed, so this is ok.
                    // We pick the maximum frequency supported by the system as the default.
                    uint64_t l_desired_cl = 0;

                    l_min_dimm_freq[l_mcs_index][l_mca_index] = fapi2::ENUM_ATTR_MSS_FREQ_MT2666;

                    uint64_t l_tCKmin = 0;

                    // Find CAS latency using JEDEC algorithm
                    FAPI_TRY( l_cas_latency.find_cl(l_desired_cl, l_tCKmin) );

                    FAPI_INF("%s. Result from CL algorithm, CL (nck): %d, tCK (ps): %d",
                             mss::c_str(l_mca), l_desired_cl, l_tCKmin);

                    l_desired_cas_latency.push_back(std::make_pair(l_desired_cl, l_mca) );

                    // Find dimm transfer speed from selected tCK
                    FAPI_TRY( mss::ps_to_freq(l_tCKmin, l_min_dimm_freq[l_mcs_index][l_mca_index]),
                              "%s. Failed ps_to_freq()", mss::c_str(l_mca) );

                    FAPI_INF("DIMM speed from selected tCK (ps): %d for %s",
                             l_min_dimm_freq[l_mcs_index][l_mca_index],
                             mss::c_str(l_mca));

                    FAPI_INF("%s. Selected DIMM speed from supported speeds: %d",
                             mss::c_str(l_mca), l_min_dimm_freq[l_mcs_index][l_mca_index]);

                }// end else
            } // mca

            FAPI_TRY(mss::set_CL_attr(l_mcs, l_desired_cas_latency),
                     "%s. Failed set_CL_attr()", mss::c_str(i_target) );

        } // close for each mcs

        FAPI_TRY(mss::set_freq_attrs(l_mcbist, l_min_dimm_freq),
                 "%s. Failed set_freq_attrs()", mss::c_str(i_target) );

        // Check MEM/NEST frequency ratio
        FAPI_TRY( mss::freq_pb_mhz(l_nest_freq) );
        FAPI_TRY( mss::freq(l_mcbist, l_mss_freq) );
        FAPI_TRY( mss::workarounds::check_dimm_nest_freq_ratio(l_mcbist, l_mss_freq, l_nest_freq) );

    fapi_try_exit:
        return fapi2::current_err;

    }// p9_mss_freq
}// extern C
