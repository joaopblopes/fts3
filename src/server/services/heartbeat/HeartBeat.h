/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include <ctime>
#include "../BaseService.h"


namespace fts3 {
namespace server {

extern time_t retrieveRecords;
extern time_t updateRecords;
extern time_t stallRecords;

class HeartBeat: public BaseService
{
public:
    HeartBeat();
    virtual void runService();

    bool isLeadNode();

private:
    unsigned index, count, start, end;

    bool criticalThreadExpired(time_t retrieveRecords, time_t updateRecords,
            time_t stallRecords);

    void orderedShutdown();
};

} // end namespace server
} // end namespace fts3

#endif // HEARTBEAT_H_
