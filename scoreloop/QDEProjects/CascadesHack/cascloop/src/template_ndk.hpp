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
		const char* email;			/* Pointless? */
		const char* nationality;	/* Pointless / Depreciated? */
	} UserInfo_t;

	typedef struct AppData_tag {
	    SC_Client_h client;
	    SC_Score_h score;
	    SC_UserController_h userController;
	    SC_ScoreController_h scoreController;
	    SC_ScoresController_h scoresController;
	    SC_LocalAchievementsController_h achievementsController;
	    SC_ChallengeController_h challengeController;
	    SC_ChallengesController_h challengesController;
	    UserInfo_t *UserInfo;	/* Todo: Turn into dynamic array */
	} AppData_t;

	void init();
	void kill();
	void userget();

	// The extension methods are defined here
	std::string start();
	void stop();

	void getuser(AppData_t *);

	std::string templateStartThread();
	std::string templateStopThread();
	bool isThreadHalt();
	void templateThreadCallback();


#endif /* TEMPLATENDK_H_ */
