/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/mbox/ipcSp.C $                                        */
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
#include <mbox/ipc_msg_types.H>
#include "ipcSp.H"
#include <runtime/runtime.H>
#include <vfs/vfs.H>
#include <mbox/ipc_reasoncodes.H>
#include <mbox/mboxif.H>
#include <errl/errlmanager.H>
#include <mbox/mbox_reasoncodes.H>
#include <intr/interrupt.H>
#include <initservice/initserviceif.H>
#include <sbeio/sbeioif.H>
#include <util/utiltce.H>
#include <targeting/targplatutil.H>

namespace ISTEP_21
{
    extern errlHndl_t callShutdown ( uint64_t i_hbInstance,
                                     bool i_masterInstance,
                                     const uint64_t i_commBase );

    extern errlHndl_t callCheckFreqAttrData(uint64_t freqData1, uint64_t freqData2);
};

trace_desc_t* g_trac_ipc = NULL;
TRAC_INIT(&g_trac_ipc, IPC_TRACE_NAME, KILOBYTE);

using namespace IPC;
using namespace ERRORLOG;
using namespace TARGETING;

IpcSp::IpcSp()
    :
        iv_msgQ()
{
}

IpcSp::~IpcSp()
{
    msg_q_destroy(iv_msgQ);
}

void IpcSp::init(errlHndl_t & o_errl)
{
    o_errl = Singleton<IpcSp>::instance()._init();
}

void* IpcSp::msg_handler(void *unused)
{
    Singleton<IpcSp>::instance().msgHandler();
    return NULL;
}

errlHndl_t IpcSp::_init()
{
    errlHndl_t err = NULL;

    iv_msgQ = msg_q_create();
    err = MBOX::msgq_register(MBOX::HB_IPC_MSGQ,iv_msgQ);


    if(!err)
    {
        task_create(IpcSp::msg_handler, NULL);
    }

    return err;
}

void IpcSp::msgHandler()
{
    errlHndl_t err = NULL;
    bool mod_loaded = false;

    while(1)
    {
        msg_t* msg = msg_wait(iv_msgQ);

        switch(msg->type)
        {
            case IPC_POPULATE_TPM_INFO_BY_NODE:
            {
                // make sure runtime module is loaded
                if ( !VFS::module_is_loaded( "libruntime.so" ) )
                {
                    err = VFS::module_load( "libruntime.so" );

                    if ( err )
                    {
                        TRACFCOMP( g_trac_ipc,
                                   "Could not load runtime module - must shutdown now!!!" );
                    }
                    else
                    {
                        mod_loaded = true;
                    }
                }

                if(!err)
                {
                    // msg->extra_data contains PAYLOAD Base
                    RUNTIME::setPayloadBaseAddress(
                               reinterpret_cast<uint64_t>(msg->extra_data));

                    // msg->data[0] contains the HDAT TPM info instance
                    // to populate
                    err = RUNTIME::populate_TpmInfoByNode(msg->data[0]);
                    if(err)
                    {
                        TRACFCOMP( g_trac_ipc, ERR_MRK"IpcSp::msgHandler: populate_TpmInfoByNode errored - must shutdown now!!!");
                    }
                }

                if (err)
                {
                    const auto l_errPlid = err->plid();
                    errlCommit(err, IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }

                if(mod_loaded)
                {
                    err = VFS::module_unload( "libruntime.so" );

                    if (err)
                    {
                        errlCommit(err, IPC_COMP_ID);
                    }
                    mod_loaded = false;
                }

                // give a response back to the sender
                err = MBOX::send(MBOX::HB_POP_TPM_INFO_MSGQ, msg, msg->data[1]);
                if (err)
                {
                    const auto l_errPlid = err->plid();
                    errlCommit(err,IPC_COMP_ID);
                    msg_free(msg);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }
                break;
            }

            case IPC_POPULATE_ATTRIBUTES:
                // make sure runtime module is loaded
                if ( !VFS::module_is_loaded( "libruntime.so" ) )
                {
                    err = VFS::module_load( "libruntime.so" );

                    if ( err )
                    {
                        TRACFCOMP( g_trac_ipc,
                                   "Could not load runtime module" );
                    }
                    else
                    {
                        mod_loaded = true;
                    }
                }

                if(!err)
                {
                    // msg->extra_data contains PAYLOAD Base
                    RUNTIME::setPayloadBaseAddress(
                        reinterpret_cast<uint64_t>(msg->extra_data));

                    // msg->data[0] contains the node number
                    err = RUNTIME::populate_HbRsvMem( msg->data[0] );
                }

                if (err)
                {
                    TRACFCOMP( g_trac_ipc, "In ipcSp: populate_node_attribute errored - must shutdown now!!!");
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err, IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }

                if(mod_loaded)
                {
                    err = VFS::module_unload( "libruntime.so" );

                    if (err)
                    {
                        errlCommit(err, IPC_COMP_ID);
                    }
                    mod_loaded = false;
                }

                // Respond
                err = MBOX::send(MBOX::HB_POP_ATTR_MSGQ, msg, msg->data[1] );
                if (err)
                {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err,IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }
                break;

            case IPC_TEST_CONNECTION:

                TRACFCOMP( g_trac_ipc,
                           "IPC received the test connection msg - %d:%d",
                            msg->data[0], msg->data[1] );

                // Tell this HB node about the other HB node
                err = INTR::addHbNode(msg->data[1]);
                if( err)
                {
                    errlCommit(err,IPC_COMP_ID);
                }

                //Send a response to indicate the connection has been
                //established
                err = MBOX::send(MBOX::HB_COALESCE_MSGQ, msg, msg->data[1] );

                if (err)
                {
                    errlCommit(err,IPC_COMP_ID);
                }
                break;

            case IPC_QUERY_CHIPINFO:
            {
                TARGETING::TargetHandleList l_procChips;
                getAllChips( l_procChips, TARGETING::TYPE_PROC , true);
                uint64_t l_systemFabricConfigurationMap = 0x0;
                for(auto l_proc : l_procChips)
                {
                    //Get fabric info from proc
                    uint8_t l_fabricChipId =
                      l_proc->getAttr<TARGETING::ATTR_FABRIC_CHIP_ID>();
                    uint8_t l_fabricGroupId =
                      l_proc->getAttr<TARGETING::ATTR_FABRIC_GROUP_ID>();
                    //Calculate what bit position this will be
                    uint8_t l_bitPos = l_fabricChipId + (RUNTIME::MAX_PROCS_PER_NODE * l_fabricGroupId);

                    //Set the bit @ l_bitPos to be 1 because this is a functional proc
                    l_systemFabricConfigurationMap |= (0x8000000000000000 >> l_bitPos);
                }

                TRACFCOMP( g_trac_ipc,
                           "IPC Query ChipInfo 0x%lx", l_systemFabricConfigurationMap);

                //Send a response with this HB instances chip info
                msg->extra_data = reinterpret_cast<uint64_t*>(l_systemFabricConfigurationMap);
                err = MBOX::send(MBOX::HB_SBE_SYSCONFIG_MSGQ, msg, msg->data[1] );
                if (err)
                {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err,IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }
                break;
             }
             case IPC_SET_SBE_CHIPINFO:
             {
                //Need to send System Configuration down to SBE
                //Master sends info to set into SBE via msg->extra_data
                uint64_t l_systemFabricConfigurationMap =
                        reinterpret_cast<uint64_t>(msg->extra_data);

                TRACFCOMP( g_trac_ipc,
                           "sending systemConfig[0x%lx] to SBEs",
                           l_systemFabricConfigurationMap);

                TARGETING::TargetHandleList l_procChips;
                getAllChips( l_procChips, TARGETING::TYPE_PROC , true);
                for(auto l_proc : l_procChips)
                {
                    TRACDCOMP( g_trac_ipc,
                               "calling sendSystemConfig on proc 0x%x",
                               TARGETING::get_huid(l_proc));
                    err = SBEIO::sendSystemConfig(l_systemFabricConfigurationMap,
                                                    l_proc);
                    if ( err )
                    {
                        TRACFCOMP( g_trac_ipc,
                                   "sendSystemConfig ERROR : Error sending sbe chip-op to proc 0x%.8X. Returning errorlog, reason=0x%x",
                                   TARGETING::get_huid(l_proc),
                                   err->reasonCode() );
                        break;
                    }
                }

                //If error terminate here
                if(err)
                {
                    TRACFCOMP( g_trac_ipc, "In ipcSp: SBEIO::sendSystemConfig errored - must shutdown now!!!");
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err, IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }
                else
                {
                    TRACFCOMP( g_trac_ipc,
                               "Successfully sent all system configs to procs via SBE chip op !!");
                }

                //Send response back to the master HB to indicate successful configuration down to SBE
                err = MBOX::send(MBOX::HB_SBE_SYSCONFIG_MSGQ, msg, msg->data[1] );
                if (err)
                {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err,IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }
                break;
             }

            case IPC_FREQ_ATTR_DATA:
            {

                TRACFCOMP( g_trac_ipc,
                           "IPC received the IPC_FREQ_ATTR_DATA msg - %u:%u",
                            msg->data[0]>>32, (msg->data[0]& 0xFFFFFFFF));

                const int NUM_MOD = 2;
                const char * mods[NUM_MOD] =
                   { "libistep21.so","libruntime.so"};
                bool loaded_mods[NUM_MOD] = {false, false};
                for (auto cnt = 0; cnt < NUM_MOD; ++cnt)
                {
                    if ( !VFS::module_is_loaded( mods[cnt] ) )
                    {
                        err = VFS::module_load( mods[cnt] );

                        if ( err )
                        {
                            TRACFCOMP( g_trac_ipc,
                                       "Could not load %s module", mods[cnt] );
                            break;
                        }
                        else
                        {
                            loaded_mods[cnt] = true;
                        }
                    }
                }

                if(!err)
                {
                    uint64_t l_freqData1 =
                        reinterpret_cast<uint64_t>(msg->data[1]);

                    uint64_t l_freqData2 =
                        reinterpret_cast<uint64_t>(msg->extra_data);

                    //  Function checks frequency attribute data. Returns an error
                    //  if there is a mismatch with that of HB master data
                    err = ISTEP_21::callCheckFreqAttrData(l_freqData1, l_freqData2);
                }

                if (err)
                {
                   uint32_t l_errPlid = err->plid();
                   errlCommit(err,IPC_COMP_ID);
                   INITSERVICE::doShutdown(l_errPlid, true);
                }

                for (auto cnt = 0; cnt < NUM_MOD; ++cnt)
                {
                    if ( loaded_mods[cnt] )
                    {
                        err = VFS::module_unload( mods[cnt] );

                        if (err)
                        {
                            errlCommit(err, IPC_COMP_ID);
                        }
                        loaded_mods[cnt] = false;
                    }

                }

                 //Send response back to the master HB to indicate set freq attr successful
                 err = MBOX::send(MBOX::HB_FREQ_ATTR_DATA_MSGQ, msg, (msg->data[0]& 0xFFFFFFFF) );

                 if (err)
                 {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err,IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                 }



             break;

            }

             case IPC_START_PAYLOAD:
            {
                const int NUM_MOD = 4;
                const char * mods[NUM_MOD] =
                   {"libp9_cpuWkup.so", "libistep21.so", "libpm.so",
                   "libruntime.so"};
                bool loaded_mods[NUM_MOD] = {false, false, false};
                for (auto cnt = 0; cnt < NUM_MOD; ++cnt)
                {
                    if ( !VFS::module_is_loaded( mods[cnt] ) )
                    {
                        err = VFS::module_load( mods[cnt] );

                        if ( err )
                        {
                            TRACFCOMP( g_trac_ipc,
                                       "Could not load %s module", mods[cnt] );
                            break;
                        }
                        else
                        {
                            loaded_mods[cnt] = true;
                        }
                    }
                }

                if (err) break;

                if(!err)
                {
                    //  Function will not return unless error
                    err = ISTEP_21::callShutdown(msg->data[0],false,
                                                                  msg->data[1]);
                }

                if(err)
                {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err, IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }

                for (auto cnt = 0; cnt < NUM_MOD; ++cnt)
                {
                    if ( loaded_mods[cnt] )
                    {
                        err = VFS::module_unload( mods[cnt] );

                        if (err)
                        {
                            errlCommit(err, IPC_COMP_ID);
                        }
                        loaded_mods[cnt] = false;
                    }

                }

                msg_free(msg);

                break;
            }

            case IPC_CLOSE_TCES:
            {
               TRACFCOMP( g_trac_ipc,
                          "Closing TCEs on this node (%d)",
                          TARGETING::UTIL::getCurrentNodePhysId());

                // make sure util module is loaded
                const char * libutil = "libutil.so";
                if ( !VFS::module_is_loaded( libutil ) )
                {
                    err = VFS::module_load( libutil );

                    if ( err )
                    {
                        TRACFCOMP( g_trac_ipc,
                                   "Could not load util module" );
                    }
                    else
                    {
                        mod_loaded = true;
                    }
                }

                if(!err)
                {
                    err = TCE::utilClosePayloadTces();
                }

                if(!err)
                {
                    err = TCE::utilDisableTces();
                }

                if(err)
                {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err, IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }

                if(mod_loaded)
                {
                    err = VFS::module_unload( libutil );

                    if (err)
                    {
                        errlCommit(err, IPC_COMP_ID);
                    }
                    else
                    {
                        mod_loaded = false;
                    }
                }

                // Respond
                err = MBOX::send(MBOX::HB_CLOSE_TCES_MSGQ, msg, msg->data[1] );
                if (err)
                {
                    uint32_t l_errPlid = err->plid();
                    errlCommit(err,IPC_COMP_ID);
                    INITSERVICE::doShutdown(l_errPlid, true);
                }

                break;
            }

            default:

                TRACFCOMP( g_trac_ipc,
                           "IPC received an unexpected message type of %d",
                           msg->type);

                /*@ errorlog tag
                 * @errortype  ERRL_SEV_INFORMATIONAL
                 * @moduleid   IPC::MOD_IPCSP_MSGHDLR
                 * @reasoncode IPC::RC_INVALID_MSG_TYPE
                 * @userdata1  Message type
                 * @userdata2  Data word 0 of message
                 *
                 * @devdesc    IPC service provider received an unexpected
                 *             message.
                 *
                 */
                err = new ERRORLOG::ErrlEntry
                    (
                     ERRORLOG::ERRL_SEV_INFORMATIONAL,    // severity
                     IPC::MOD_IPCSP_MSGHDLR,              // moduleid
                     IPC::RC_INVALID_MSG_TYPE,            // reason code
                     msg->type,
                     msg->data[0],
                     true //Add HB Software Callout
                    );
                //@todo: RTC:93750 Create real parseable FFDC class
                err->addFFDC(MBOX_COMP_ID,
                             msg,
                             sizeof(msg_t),
                             1,//version
                             MBOX::MBOX_UDT_MSG_DATA);//subsect

                err->collectTrace(MBOXMSG_TRACE_NAME);
                err->collectTrace(IPC_TRACE_NAME);

                errlCommit(err, IPC_COMP_ID);

                msg_free(msg);

                break;
        }
    }
}


