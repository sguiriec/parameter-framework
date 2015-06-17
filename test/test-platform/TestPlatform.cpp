/*
 * Copyright (c) 2011-2015, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TestPlatform.h"
#include "ParameterMgrPlatformConnector.h"
#include "RemoteProcessorServer.h"

using std::string;

namespace test
{
namespace platform
{

TestPlatform::TestPlatform(const string& configurationFile, int portNumber, sem_t& exitSemaphore) :
    mParameterMgrPlatformConnector(configurationFile),
    mParameterMgrPlatformConnectorLogger(),
    mCommandParser(*this),
    mRemoteProcessorServer(portNumber, mCommandParser.getCommandHandler()),
    mExitSemaphore(exitSemaphore)
{
    mParameterMgrPlatformConnector.setLogger(&mParameterMgrPlatformConnectorLogger);
}

bool TestPlatform::load(std::string& error)
{
    // Start remote processor server
    if (!mRemoteProcessorServer.start(error)) {

        error = "TestPlatform: Unable to start remote processor server: " + error;
        return false;
    }

    return true;
}

bool TestPlatform::setCriterionState(std::string criterionName,
                                      const IRemoteCommand& remoteCommand,
                                      string& result)
{
    core::criterion::Criterion* pCriterion =
        mParameterMgrPlatformConnector.getCriterion(criterionName);

    if (!pCriterion) {

        result = "Unable to retrieve selection criterion: " + criterionName;
        return false;
    }

    // Get substate number, the first argument (index 0) is the criterion name
    uint32_t uiNbSubStates = remoteCommand.getArgumentCount() - 1;

    core::criterion::State state{};
    for (uint32_t i = 1; i <= uiNbSubStates; i++) {
        state.emplace(remoteCommand.getArgument(i));
    }

    // Set criterion new state
    if (!pCriterion->setState(state, result)) {
        return false;
    }
    return true;
}

} /** platform namespace */
} /** test namespace */
