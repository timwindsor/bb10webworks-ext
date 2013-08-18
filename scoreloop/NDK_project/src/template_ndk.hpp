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

#define SBSL_BAD_PARAM 32888
#define ISXDIGIT(x) (isxdigit((int) ((unsigned char) x)))

typedef struct LeaderInfo_tag {
	SC_User_h user;
	const char* login;
	double majorScore;
	double minorScore;
	unsigned int mode;
	unsigned int level;
	unsigned int rank;
} LeaderInfo_t;

typedef struct GameInfo_tag {
	const char* name;
	const char* imgurl;
	const char* publisher;
	const char* version;
	const char* dlurl;
	const char* desc;
	unsigned int modes;
} GameInfo_t;

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
} UserInfo_t;

typedef struct ChallengeInfo_tag {
	SC_User_h contender;
	const char* contenderlogin;
	SC_Score_h contenderscore;
	SC_User_h contestant;
	const char* contestantlogin;
	SC_Score_h contestantscore;
	SC_Money_h stake;
	SC_Money_h prize;
	const char* created;
	unsigned int mode;
	unsigned int level;
} ChallengeInfo_t;

typedef struct AppData_tag {
    void *m_pTemplateNDK;

	SC_Client_h client;
	SC_Score_h score;
	SC_Challenge_h challenge;
	SC_Money_h money;

	SC_LocalAchievementsController_h achievementsController;
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

	unsigned int rank;
	unsigned long cash;

	UserInfo_t *UserInfo;
	unsigned int buddies_c;
	UserInfo_t **buddies;
	unsigned int leaders_c;
	LeaderInfo_t **leaders;
	unsigned int games_c;
	GameInfo_t **games;
	unsigned int challenges_c;
	ChallengeInfo_t **challenges;
} AppData_t;


class TemplateJS;

namespace webworks {

class TemplateNDK {
public:
	explicit TemplateNDK(TemplateJS *parent = NULL);
	virtual ~TemplateNDK();

	// The extension methods are defined here
	std::string readLog();

	std::string start(const std::string& arg);

	std::string getUser();
	void getUserCallback(AppData_t *app, SC_Error_t rc);

	std::string getBuddyList();
	void getBuddyListCallback(AppData_t *app, SC_Error_t rc);

	std::string getLeaders(const std::string& arg);
	void getLeadersCallback(AppData_t *app, SC_Error_t rc);

	std::string setChallenge(const std::string& arg);
	void setChallengeCallback(AppData_t *app, SC_Error_t rc);
	std::string setChallengeScore(const std::string& arg);
	void setChallengeScoreCallback(AppData_t *app, SC_Error_t rc);

	std::string getChallengeList();
	void getChallengeListCallback(AppData_t *app, SC_Error_t rc);

	std::string setScore(const std::string& arg);
	void setScoreCallback(AppData_t *app, SC_Error_t rc);

	bool isThreadHalt();
	std::string templateStartThread();
	std::string templateStopThread();
	void templateThreadCallback();

private:
	TemplateJS *m_pParent;
	int templateProperty;
	bool threadHalt;
	pthread_t m_thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

} // namespace webworks

#endif /* TEMPLATENDK_H_ */
