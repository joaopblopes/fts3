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

#include "RestContextAdapter.h"

#include "ui/SubmitTransferCli.h"
#include "rest/HttpRequest.h"

using namespace fts3::cli;

static bool isJobFinished(const std::string &state)
{
    return state == "FAILED" || state == "FINISHED" || state == "FINISHEDDIRTY" || state == "CANCELED";
}

/**
 * This is the entry point for the fts3-transfer-submit command line tool.
 */
int main(int ac, char* av[])
{
    try
        {
            SubmitTransferCli cli;
            cli.parse(ac, av);
            if (cli.printHelp()) return 0;
            cli.validate();

            RestContextAdapter ctx(cli.getService(), cli.capath(), cli.proxy(), cli.isInsecure());

            cli.printApiDetails(ctx);

            // delegate the credential if necessary
            ctx.delegate(cli.getDelegationId(), cli.getExpirationTime());

            std::string jobId;
            std::vector<File> files = cli.getFiles();
            std::map<std::string, std::string> params = cli.getParams();
            // submit the job
            jobId = ctx.transferSubmit (
                        files,
                        params,
                        cli.getExtraParameters()
                    );

            MsgPrinter::instance().print("", "job_id", jobId);

            // check if the -b option has been used
            if (cli.isBlocking())
                {
                    std::string status;
                    // wait until the transfer is ready
                    do
                        {
                            sleep(2);
                            status = ctx.getTransferJobStatus(jobId, false).getStatus();
                        }
                    while (!isJobFinished(status));
                }

        }
    catch(cli_exception const & ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(std::exception& ex)
        {
            MsgPrinter::instance().print(ex);
            return 1;
        }
    catch(...)
        {
            MsgPrinter::instance().print("error", "exception of unknown type!");
            return 1;
        }

    return 0;
}
