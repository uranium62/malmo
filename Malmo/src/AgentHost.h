// --------------------------------------------------------------------------------------------------
//  Copyright (c) 2016 Microsoft Corporation
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
//  associated documentation files (the "Software"), to deal in the Software without restriction,
//  including without limitation the rights to use, copy, modify, merge, publish, distribute,
//  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------------------------------------------------------------

#ifndef _AGENTHOST_H_
#define _AGENTHOST_H_

// Local:
#include "ArgumentParser.h"
#include "ClientConnection.h"
#include "ClientPool.h"
#include "MissionInitSpec.h"
#include "MissionRecord.h"
#include "MissionSpec.h"
#include "StringServer.h"
#include "VideoServer.h"
#include "WorldState.h"

// Boost:
#include <boost/thread.hpp>

// STL:
#include <string>

namespace malmo
{
    //! An agent host mediates between the researcher's code (the agent) and the Mod (the target environment).
    class AgentHost : public ArgumentParser
    {
        public:

            //! Specifies what to do when there are more video frames being received than can be processed.
            enum VideoPolicy { 
                  LATEST_FRAME_ONLY          //!< Discard all but the most recent frame. This is the default.
                , KEEP_ALL_FRAMES            //!< Attempt to store all of the frames.
            };

            //! Specifies what to do when there are more rewards being received than can be processed.
            enum RewardsPolicy { 
                  LATEST_REWARD_ONLY         //!< Discard all but the most recent reward.
                , SUM_REWARDS                //!< Add up all the rewards received. This is the default.
                , KEEP_ALL_REWARDS           //!< Attempt to store all the rewards.
            };

            //! Specifies what to do when there are more observations being received than can be processed.
            enum ObservationsPolicy { 
                  LATEST_OBSERVATION_ONLY    //!< Discard all but the most recent observation. This is the default.
                , KEEP_ALL_OBSERVATIONS      //!< Attempt to store all the observations.
            };

            //! Creates an agent host with default settings.
            AgentHost();

            //! Destructor
            ~AgentHost();

            //! Starts a mission running. Throws an exception if something goes wrong.
            //! \param mission The mission specification.
            //! \param client_pool A list of the Minecraft instances that can be used.
            //! \param mission_record The specification of the mission recording to make.
            //! \param role Index of the agent that this agent host is to manage. Zero-based index. Use zero if there is only one agent in this mission.
            //! \param unique_experiment_id An arbitrary identifier that is used to disambiguate our mission from other runs.
            void startMission( const MissionSpec& mission, const ClientPool& client_pool, const MissionRecordSpec& mission_record, int role, std::string unique_experiment_id );

            //! Starts a mission running, in the simple case where there is only one agent running on the local machine. Throws an exception if something goes wrong.
            //! \param mission The mission specification.
            //! \param mission_record The specification of the mission recording to make.
            void startMission(const MissionSpec& mission, const MissionRecordSpec& mission_record);

            //! Gets the latest world state received from the game.
            //! \returns The world state.
            WorldState peekWorldState() const;

            //! Gets the latest world state received from the game and resets it to empty.
            //! \returns The world state.
            WorldState getWorldState();

            //! Specifies how you want to deal with multiple video frames.
            //! \param videoPolicy How you want to deal with multiple video frames coming in asynchronously.
            void setVideoPolicy(VideoPolicy videoPolicy);

            //! Specifies how you want to deal with multiple rewards.
            //! \param rewardsPolicy How you want to deal with multiple rewards coming in asynchronously.
            void setRewardsPolicy(RewardsPolicy rewardsPolicy);

            //! Specifies how you want to deal with multiple observations.
            //! \param observationsPolicy How you want to deal with multiple observations coming in asynchronously.
            void setObservationsPolicy(ObservationsPolicy observationsPolicy);
            
            //! Sends a command to the game client.
            //! See the mission handlers documentation for the permitted commands for your chosen command handler.
            //! \param command The command to send as a string. e.g. "move 1"
            void sendCommand(std::string command);

            //! Returns a pointer to the current MissionInitSpec, to allow retrieval of the ports being used.
            //! (If port 0 is requested this means bind to any port that is available.)
            //! \returns A shared pointer to the MissionInitSpec.
            boost::shared_ptr<MissionInitSpec> getMissionInit();
            const boost::shared_ptr<MissionInitSpec> getMissionInit() const;

            friend std::ostream& operator<<(std::ostream& os, const AgentHost& ah);
        private:
        
            void initializeOurServers(const MissionSpec& mission, const MissionRecordSpec& mission_record, int role, std::string unique_experiment_id);
            std::string generateMissionInit();
            void searchThroughClientPool( const ClientPool& client_pool, bool looking_for_server );
        
            void listenForMissionControlMessages( int port );
            void listenForVideo( int port, short width, short height, short channels );
            void listenForRewards( int port );
            void listenForObservations( int port );
            
            void onMissionControlMessage(TimestampedString message);
            void onVideo(TimestampedVideoFrame message);
            void onReward(TimestampedString message);
            void onObservation(TimestampedString message);
            
            void openCommandsConnection();

            void close();
            
            void processReceivedReward( TimestampedReward reward );
            
            boost::asio::io_service io_service;
            boost::shared_ptr<StringServer>   mission_control_server;
            boost::shared_ptr<VideoServer>    video_server;
            boost::shared_ptr<StringServer>   rewards_server;
            boost::shared_ptr<StringServer>   observations_server;
            boost::optional<boost::asio::io_service::work> work;
            std::vector<boost::shared_ptr<boost::thread>> background_threads;

            boost::shared_ptr<ClientConnection> commands_connection;
            std::ofstream commands_stream;

            VideoPolicy        video_policy;
            RewardsPolicy      rewards_policy;
            ObservationsPolicy observations_policy;
            
            WorldState world_state;
            mutable boost::mutex world_state_mutex;
            
            boost::shared_ptr<MissionInitSpec> current_mission_init;
            boost::shared_ptr<MissionRecord> current_mission_record;
            int current_role;
    };

}

#endif
