/*
* Copyright 2013 Research In Motion Limited.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef TEMPLATENDK_HPP_
#define TEMPLATENDK_HPP_

#include <string>
#include <pthread.h>
#include <scoreloop/scoreloopcore.h>

typedef struct UserInfo_tag {
	SC_User_h user;
	const char* login;
	const char* imgurl;
	int buddy_c;
	int games_c;
	int achievements_c;
	SC_Bool_t challenge;
	SC_UserState_t state;
	SC_UserHandle_h handle;
	SC_Context_h ctx;
	void** buddies;
	const char* email;			/* Pointless? */
} UserInfo_t;

typedef struct AppData_tag {
	SC_Client_h client;
	SC_Score_h score;
	SC_AchievementsController_h achievementsController;
	SC_ActivitiesController_h activitiesController;
	SC_ChallengeController_h challengeController;
	SC_ChallengesController_h challengesController;
	SC_GameItemController_h gameItemController;
	SC_GameItemsController_h gameItemsController;
	SC_GamesController_h gamesController;
	SC_LocalAchievementsController_h localAchievementsController;
	SC_MessageController_h messageController;
	SC_RankingController_h rankingController;
	SC_ScoreController_h scoreController;
	SC_ScoresController_h scoresController;
	SC_UserController_h userController;
	SC_UsersController_h usersController;
	UserInfo_t *UserInfo;
	int buddy_c;
	UserInfo_t **buddies;
} AppData_t;


class TemplateJS;

namespace webworks {

class TemplateNDK {
public:
	explicit TemplateNDK(TemplateJS *parent = NULL);
	virtual ~TemplateNDK();

	// The extension methods are defined here
	std::string start();
	void stop();

	SC_Error_t scgetuser(AppData_t *app);
	SC_Error_t scgetbuddies(AppData_t *app);

	std::string templateStartThread();
	std::string templateStopThread();
	bool isThreadHalt();
	void templateThreadCallback();

private:
	TemplateJS *m_pParent;
	int templateProperty;
	bool threadHalt;
	pthread_t m_thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	AppData_t app;
};

} // namespace webworks

#endif /* TEMPLATENDK_H_ */
