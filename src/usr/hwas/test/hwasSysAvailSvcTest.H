/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/hwas/test/hwasSysAvailSvcTest.H $                     */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2013,2018                        */
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
#ifndef _HWASSYSAVAILTEST_H
#define _HWASSYSAVAILTEST_H

/**
 *  @file hwasSysAvailTest.H
 *
 *  @brief Unit tests for HWAS System Availability Test.
 */

//******************************************************************************
// Includes
//******************************************************************************

// CXXTEST
#include <cxxtest/TestSuite.H>
#include <errl/errlmanager.H>
#include <targeting/common/targetservice.H>
#include <hwas/common/hwas_reasoncodes.H>
#include <hwas/common/hwasCommon.H>
#include <targeting/common/utilFilter.H>


//  $$  make sure this is disabled before checking in!!
#define DISABLE_UNIT_TESTS    1
//$$#define DISABLE_UNIT_TESTS    0

using   namespace   TARGETING;

class HwasSysAvailTest: public CxxTest::TestSuite
{
public:

    enum    TargetState_t  {
        RESTORE         =   0,
        SET_NONFUNC     =   1,
    };


    /*
     *  @brief  Set all or most of the targets in the target list to
     *      i_state ==  SET_NONFUNC :   non-functional
     *      i_state ==  RESTORE     :   back to its' original state
     *  Stores the original State in i_origStates; it is assumed that this
     *  will be called first with SET_NONFUNC to fill in i_origStates.
     *  @param  i_state         - SET_NONFUNC or RESTORE
     *  @param  i_targetHList   -   TargetHandleList of targets to work on
     *  @param  i_origStates    -   array of HWasStates to store the original
     *                              state in.
     *  @param  l_keep          -   number of states to keep (i.e. leave alone)
     *                              when running SET_NONFUNC
     *
     **/

    void    setTargetStates( TargetState_t          i_state,
                             TargetHandleList     & i_targetHList,
                             TARGETING::HwasState   *i_origStates,
                             uint8_t                l_keep=0
                             )
    {

        TARGETING::HwasState    l_fakeState;

        for( uint8_t i = l_keep;
             i < i_targetHList.size();
             i++ )
        {
            TARGETING::Target * l_target  =   i_targetHList[i];

            if ( i_state  == SET_NONFUNC  )
            {
                //  set nonfunctional
                i_origStates[i] =
                    l_target->getAttr<TARGETING::ATTR_HWAS_STATE>();
                l_fakeState =   i_origStates[i];
                TS_TRACE( "setTargetStates: HwasState fakeState: "
                          "deconf=0x%x,PO=0x%x,pres=0x%x,func=0x%x,dumpf=0x%x",
                          l_fakeState.deconfiguredByEid,
                          l_fakeState.poweredOn,
                          l_fakeState.present,
                          l_fakeState.functional,
                          l_fakeState.dumpfunctional
                          );

                l_fakeState.functional  =   0 ;

                l_target->setAttr<TARGETING::ATTR_HWAS_STATE>(l_fakeState);
            }
            else
            {
                // Get fake HWAS state setting
                l_fakeState = l_target->getAttr<TARGETING::ATTR_HWAS_STATE>();

                // Restore to original state.
                l_target->setAttr<TARGETING::ATTR_HWAS_STATE>(i_origStates[i]);
            }
        }   //  endfor
    }

    /**
     *  @brief mark all cores nonfunctional and then call checkMinimumHardware()
     */
    void testCheckMinimumHardwareCore()
    {
#if DISABLE_UNIT_TESTS

        //  NOTE:  These tests mark targets nonfunctional and then
        //  restore them.  Since all the unit tests run in parallel, this
        //  may cause other tests to fail.
        //  Do not leave this test enabled for normal operations.
        TS_TRACE(INFO_MRK "SKIPPING:  other tests could be affected.");
        HWAS_INF("testCheckMinimumHardwareCore: Skipped");
#else

        errlHndl_t  l_errl  =   NULL;
        TS_TRACE(INFO_MRK "testCheckMinimumHardwareCore");
        HWAS_INF("testCheckMinimumHardwareCore: Started");

        TargetHandleList l_cores;
        getAllChiplets(l_cores, TYPE_CORE, true );
        TS_TRACE( "testCheckMinimumHardwareCore: %d functional cores",
                  l_cores.size() );
        TARGETING::HwasState    l_origStates[ l_cores.size() ]  ;

        TS_TRACE( "testCheckMinimumHardwareCore: set cores nonfunctional");
        setTargetStates( SET_NONFUNC, l_cores, &l_origStates[0]  );

        // Save original Block Spec Deconfig value
        Target * pSys;
        targetService().getTopLevelTarget(pSys);
        TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_orig_block_spec_deconfig =
            pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();

        // If original attribute value is non-zero, clear it
        if(l_orig_block_spec_deconfig != 0)
        {
            // Clear attribute to test that checkMinimumHardware() sets it
            pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(0);
        }

        TS_TRACE( "testCheckMinimumHardwareCore: check for minimum hardware" );
        l_errl  =   HWAS::checkMinimumHardware();
        if ( l_errl )
        {
            TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_block_spec_deconfig =
                pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();
            if(l_block_spec_deconfig == 0)
            {
                TS_FAIL("testCheckMinimumHardwareCore: Expected non-zero value "
                        "for ATTR_BLOCK_SPEC_DECONFIG");
            }
            else
            {
                //  got an errorlog, no cores
                TS_TRACE( "testCheckMinimumHardwareCore: PASSED");
            }
            //  Commit the errorlog.
            errlCommit( l_errl, HWAS_COMP_ID );
        }
        else
        {
            //  no errorlog, bad news
            TS_FAIL( "testCheckMinimumHardwareCore:  FAILED, no errlog" );
        }

        TS_TRACE( "testCheckMinimumHardwareCore: restore original states");
        setTargetStates( RESTORE, l_cores, &(l_origStates[0])  );
        pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(l_orig_block_spec_deconfig);

#endif
    }


    /**
     *  @brief mark all dimms nonfunctional and then call checkMinimumHardware()
     */
    void testCheckMinimumHardwareDimm()
    {
#if DISABLE_UNIT_TESTS
        //  NOTE:  These tests mark targets nonfunctional and then
        //  restore them.  Since all the unit tests run in parallel, this
        //  may cause other tests to fail.
        //  Do not leave this test enabled for normal operations.
        TS_TRACE(INFO_MRK "SKIPPING:  other tests could be affected.");
        HWAS_INF("testCheckMinimumHardwareDimm: Skipped");
#else

        errlHndl_t  l_errl  =   NULL;
        TS_TRACE(INFO_MRK "testCheckMinimumHardwareDimm");
        HWAS_INF("testCheckMinimumHardwareDimm: Started");

        //  check for functional dimms
        TargetHandleList l_dimms;
        getAllLogicalCards(l_dimms, TYPE_DIMM, true );
        TS_TRACE( "testCheckMinimumHardwareDimm: %d functional dimms",
                  l_dimms.size() );

        TARGETING::HwasState    l_origStates[ l_dimms.size() ]  ;

        setTargetStates( SET_NONFUNC, l_dimms, &(l_origStates[0])  );

        // Save original Block Spec Deconfig value
        Target * pSys;
        targetService().getTopLevelTarget(pSys);
        TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_orig_block_spec_deconfig =
            pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();

        // If original attribute value is non-zero, clear it
        if(l_orig_block_spec_deconfig != 0)
        {
            // Clear attribute to test that checkMinimumHardware() sets it
            pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(0);
        }

        TS_TRACE( "testCheckMinimumHardwareDimm: check for minimum hardware" );
        l_errl  =   HWAS::checkMinimumHardware();
        if ( l_errl )
        {
            TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_block_spec_deconfig
                = pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();
            if(l_block_spec_deconfig == 0)
            {
                TS_FAIL("testCheckMinimumHardwareDimm: Expected non-zero value "
                        "for ATTR_BLOCK_SPEC_DECONFIG");
            }
            else
            {
                //  got an errorlog, no dimms
                TS_TRACE( "testCheckMinimumHardwareDimm: PASSED");
            }
            //  Commit the errorlog.
            errlCommit( l_errl, HWAS_COMP_ID );
        }
        else
        {
            //  no errorlog, bad news
            TS_FAIL( "testCheckMinimumHardwareDimm:  FAILED, no errlog" );
        }

        setTargetStates( RESTORE, l_dimms, &(l_origStates[0])  );
        pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(l_orig_block_spec_deconfig);

#endif
    }


    /**
     *  @brief mark all cores AND dimms nonfunctional and
     *      then call checkMinimumHardware()
     *  Test should fail and we should have 2 errorlogs with a common plid.
     */
    void testCheckMinimumHardwareBoth()
    {
#if DISABLE_UNIT_TESTS
        //  NOTE:  These tests mark targets  nonfunctional and then
        //  restore them.  Since all the unit tests run in parallel, this
        //  may cause other tests to fail.
        //  Do not leave this test enabled for normal operations.
        TS_TRACE(INFO_MRK "SKIPPING:  other tests could be affected.");
        HWAS_INF("testCheckMinimumHardwareBoth: Skipped");
#else

        errlHndl_t  l_errl  =   NULL;
        TS_TRACE(INFO_MRK "testCheckMinimumHardwareBoth");
        HWAS_INF("testCheckMinimumHardwareBoth: Started");

        //  set all cores nonfunctional
        TargetHandleList l_cores;
        getAllChiplets(l_cores, TYPE_CORE, true );
        TS_TRACE( "testCheckMinimumHardwareBoth: %d functional cores",
                  l_cores.size() );
        TARGETING::HwasState    l_origCoreStates[ l_cores.size() ]  ;
        TS_TRACE( "testCheckMinimumHardwareBoth: set cores nonfunctional");
        setTargetStates( SET_NONFUNC, l_cores, &l_origCoreStates[0]  );

        //  set all dimms nonfunctional
        TargetHandleList l_dimms;
        getAllLogicalCards(l_dimms, TYPE_DIMM, true );
        TS_TRACE( "testCheckMinimumHardwareBoth: %d functional dimms",
                  l_dimms.size() );
        TARGETING::HwasState    l_origDimmStates[ l_dimms.size() ]  ;
        TS_TRACE( "testCheckMinimumHardwareBoth: set dimms nonfunctional");
        setTargetStates( SET_NONFUNC, l_dimms, &(l_origDimmStates[0])  );

        // Save original Block Spec Deconfig value
        Target * pSys;
        targetService().getTopLevelTarget(pSys);
        TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_orig_block_spec_deconfig =
            pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();

        // If original attribute value is non-zero, clear it
        if(l_orig_block_spec_deconfig != 0)
        {
            // Clear attribute to test that checkMinimumHardware() sets it
            pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(0);
        }

        TS_TRACE( "testCheckMinimumHardwareBoth: check for minimum hardware" );
        l_errl  =   HWAS::checkMinimumHardware();
        if ( l_errl )
        {
            TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_block_spec_deconfig
                = pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();
            if(l_block_spec_deconfig == 0)
            {
                TS_FAIL("testCheckMinimumHardwareBoth: Expected non-zero value "
                        "for ATTR_BLOCK_SPEC_DECONFIG");
            }
            else
            {
                //  got an errorlog, this is good.
                //  There should be 2 errorlogs with a common plid.
                TS_TRACE( "testCheckMinimumHardwareBoth: PASSED");
            }
            //  Commit the errorlog.
            errlCommit( l_errl, HWAS_COMP_ID );
        }
        else
        {
            //  no errorlog, bad news
            TS_FAIL( "testCheckMinimumHardwareBoth:  FAILED, no errorlog." );
        }

        //  restore everything to normal.
        setTargetStates( RESTORE, l_cores, &(l_origCoreStates[0])  );
        setTargetStates( RESTORE, l_dimms, &(l_origDimmStates[0])  );
        pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(l_orig_block_spec_deconfig);

#endif
    }


    /**
     *  @brief mark all dimms (except 1) nonfunctional and
     *      then call checkMinimumHardware()
     *  Test should pass.
     */
    void testCheckMinimumHardwareOneDimm()
    {
#if DISABLE_UNIT_TESTS
        //  NOTE:  These tests mark targets nonfunctional and then
        //  restore them.  Since all the unit tests run in parallel, this
        //  may cause other tests to fail.
        //  Do not leave this test enabled for normal operations.
        TS_TRACE(INFO_MRK "SKIPPING:  other tests could be affected.");
        HWAS_INF("testCheckMinimumHardwareOneDimm: Skipped");
#else

        errlHndl_t  l_errl  =   NULL;
        TS_TRACE(INFO_MRK "testCheckMinimumHardwareOneDimm");
        HWAS_INF("testCheckMinimumHardwareOneDimm: Started");

        //  check for functional dimms
        TargetHandleList l_dimms;
        getAllLogicalCards(l_dimms, TYPE_DIMM, true );
        TS_TRACE( "testCheckMinimumHardwareOneDimm: %d functional dimms",
                  l_dimms.size() );

        TARGETING::HwasState    l_origStates[ l_dimms.size() ]  ;

        setTargetStates( SET_NONFUNC, l_dimms, &(l_origStates[0]), 1  );

        // Save original Block Spec Deconfig value
        Target * pSys;
        targetService().getTopLevelTarget(pSys);
        TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_orig_block_spec_deconfig =
            pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();

        // If original attribute value is non-zero, clear it
        if(l_orig_block_spec_deconfig != 0)
        {
            // Clear attribute, test that checkMinimumHardware() does NOT set it
            pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(0);
        }

        TS_TRACE( "testCheckMinimumHardwareOneDimm: "
                  "check for minimum hardware" );
        l_errl  =   HWAS::checkMinimumHardware();
        if ( l_errl )
        {
            //  got an errorlog, bad news
            TS_FAIL( "testCheckMinimumHardwareOneDimm:  FAILED " );
            errlCommit( l_errl, HWAS_COMP_ID );
        }
        else
        {
            TARGETING::ATTR_BLOCK_SPEC_DECONFIG_type l_block_spec_deconfig
                = pSys->getAttr<ATTR_BLOCK_SPEC_DECONFIG>();
            if(l_block_spec_deconfig != 0)
            {
                TS_FAIL("testCheckMinimumHardwareOneDimm: Expected zero value "
                        "for ATTR_BLOCK_SPEC_DECONFIG, not %d",
                        l_block_spec_deconfig);
            }
            else
            {
                //  No errorlog, this is good.
                TS_TRACE( "testCheckMinimumHardwareOneDimm: PASSED");
            }
        }

        setTargetStates( RESTORE, l_dimms, &(l_origStates[0]), 1  );
        pSys->setAttr<ATTR_BLOCK_SPEC_DECONFIG>(l_orig_block_spec_deconfig);

#endif
    }

};  // end class

#undef DISABLE_UNIT_TESTS

#endif
