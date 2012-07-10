/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

#pragma once

#include "server_dev.h"
#include "config/serverconfig.h"
#include "common/logger.h"
#include "web_service_handler.h"
#include <string>

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;
using namespace FTS3_CONFIG_NAMESPACE;

/* -------------------------------------------------------------------------- */

/** Class representing the FTS3 server logic. */
template <typename TRAITS> 
class GenericServer
{

public:
    /** Start the service. */
    void start()
    {
        unsigned int port = theServerConfig().get<unsigned int>("Port");
	    const std::string& ip = theServerConfig().get<std::string>("IP");

	    typename TRAITS::TransferWebServiceType handler_t;
        handler_t.listen_p(port, ip);

        typename TRAITS::ProcessServiceType processHandler;
        processHandler.executeTransfer_p();	
	
        typename TRAITS::ProcessQueueType queueHandler;
        queueHandler.executeTransfer_p();		
	
        TRAITS::ThreadPoolType::instance().wait();
    }

    /* ---------------------------------------------------------------------- */
    
    /** Stop the service. */
    void stop() 
    {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS3 server stopping...";
        TRAITS::ThreadPoolType::instance().stop();
        theLogger() << commit;
    }
};

FTS3_SERVER_NAMESPACE_END

