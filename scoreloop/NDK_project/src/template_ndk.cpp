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

#include <string>
#include <sstream>
#include <math.h>
#include <json/writer.h>
#include <json/reader.h>
#include <pthread.h>
#include "template_ndk.hpp"
#include "template_js.hpp"
#include <string.h>
#include <stdio.h>

#include <scoreloop/scoreloopcore.h>
#include <pthread.h>
#include <sys/time.h>

namespace webworks {

AppData_t app;
SC_InitData_t initData;
char* buffer = NULL;
int entry_count = 0;
FILE* fplog;
double initstamp = 0;

LeaderInfo_t* GetLeaderInfo(SC_User_h user, SC_Score_h oScore);
UserInfo_t *GetUserInfo(SC_User_h user, bool isBuddy);
char* urldecode(const char *str);
static SC_Money_h CreateMoney(AppData_t *app, unsigned int amount);

SC_Error_t RequestUser(AppData_t *app);
static void RequestUserCompletionCallback(void *userData, SC_Error_t completionStatus);
SC_Error_t SubmitScore(AppData_t *app, double result, double minorResult, unsigned int level, unsigned int mode);
static void SubmitScoreCompletionCallback(void *userData, SC_Error_t completionStatus);
SC_Error_t CreateChallenge(AppData_t *app, unsigned int amount, double result, double minorResult, unsigned int level, unsigned int mode, SC_User_h against);
static void CreateChallengeCompletionCallback(void *userData, SC_Error_t completionStatus);
SC_Error_t CreateChallengeScore(AppData_t *app, double result, double minorResult, unsigned int level, unsigned int mode);
static void CreateChallengeScoreCallback(void *userData, SC_Error_t completionStatus);
SC_Error_t LoadLeaderboard(AppData_t *app, unsigned int mode, const SC_ScoresSearchList_t searchList, unsigned int count);
static void LoadLeaderboardCompletionCallback(void *userData, SC_Error_t completionStatus);




SC_Error_t LoadChallenges(AppData_t *app);
static void LoadChallengesCompletionCallback(void *userData, SC_Error_t completionStatus);

SC_Error_t AcceptChallenge(AppData_t *app, SC_Challenge_h challenge);
static void AcceptChallengeCompletionCallback(void *userData, SC_Error_t completionStatus);

SC_Error_t SubmitChallenge(AppData_t *app, SC_Challenge_h challenge, double result, unsigned int mode);
static void SubmitChallengeCompletionCallback(void *userData, SC_Error_t completionStatus);

SC_Error_t AchieveAward(AppData_t *app, const char *awardIdentifier);
static void AchieveAwardCompletionCallback(void *userData, SC_Error_t completionStatus);

SC_Error_t LoadAchievements(AppData_t *app);
static void LoadAchievementsCompletionCallback(void *userData, SC_Error_t completionStatus);










SC_Error_t scgetbuddies(AppData_t *app);

double tstamp();


double tstamp() {
	struct timeval tv;
	double timestamp = 0;
	gettimeofday(&tv,NULL);
	timestamp = tv.tv_sec + ((double) tv.tv_usec / 1000000);
	if(initstamp == 0) {
		initstamp = timestamp;
	}

	timestamp = timestamp - initstamp;

	return timestamp;
}

TemplateNDK::TemplateNDK(TemplateJS *parent) {
	m_pParent = parent;
	m_thread = 0;
	pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	threadHalt = true;

	// Initialize the platform adaptation object
	SC_InitData_Init(&initData);

    if((fplog = fopen ("data/file.txt", "r")) != NULL) {
		long lSize;
		fseek (fplog , 0 , SEEK_END);
		lSize = ftell (fplog) + 1;
		rewind (fplog);
		if(buffer != NULL) {
			free(buffer);
		}
		buffer = (char*) calloc (1, sizeof(char)*lSize);
		fread (buffer,1,lSize,fplog);
		fseek (fplog , 0 , SEEK_END);
		fclose(fplog);
    }

    fplog = fopen ("data/file.txt", "w+");

    // Optionally modify the following fields:
	// initData.currentVersion = SC_INIT_CURRENT_VERSION;
	// initData.minimumRequiredVersion = SC_INIT_VERSION_1_0;
	// initData.runLoopType = SC_RUN_LOOP_TYPE_BPS;

}


TemplateNDK::~TemplateNDK() {
	if(!threadHalt) {
		templateStopThread();
	}
	fclose(fplog);
	if(buffer != NULL) {
		free(buffer);
	}

}

// These methods are the true native code we intend to reach from WebWorks

std::string TemplateNDK::readLog() {
	Json::FastWriter writer;
	Json::Value root;
	long lSize;

	fseek (fplog , 0 , SEEK_END);
	lSize = ftell (fplog) + 1;
	rewind (fplog);

	if(buffer != NULL) {
		free(buffer);
	}
	buffer = (char*) calloc (1, sizeof(char)*lSize);
	fread (buffer,1,lSize,fplog);


	fseek (fplog , 0 , SEEK_END);

	root["log"] = buffer;

	return writer.write(root);
}

std::string TemplateNDK::start(const std::string& arg) {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;

	entry_count++;

	if(entry_count == 1) {
		char *aGameId = NULL;
		char *aGameSecret = NULL;
		char *aGameVersion = NULL;
		char *aCurrency = NULL;
		char *aLanguageCode = NULL;
		int rc = 0;

		bool parse = reader.parse(arg, root);

		if (parse) {
			aGameId = urldecode(root["slGameId"].asCString());
			aGameSecret = urldecode(root["slGameSecret"].asCString());
			aGameVersion = urldecode(root["slGameVersion"].asCString());
			aCurrency = urldecode(root["slCurrency"].asCString());
			aLanguageCode = urldecode(root["slLanguageCode"].asCString());
		}

		memset(&app, 0, sizeof(AppData_t));

		app.m_pTemplateNDK = this;

		if(threadHalt) {
			initData.runLoopType = SC_RUN_LOOP_TYPE_CUSTOM;

			rc = SC_Client_New(&app.client, &initData, aGameId, aGameSecret, aGameVersion, aCurrency, aLanguageCode);

			if(rc == SC_OK) {
				templateStartThread();
			}
		}

		if(rc == SC_OK) {
			res["status"] = true;
		} else {
			res["status"] = false;
		}

		res["aGameId"] = aGameId;
		res["aGameSecret"] = aGameSecret;
		res["aGameVersion"] = aGameVersion;
		res["aCurrency"] = aCurrency;
		res["aLanguageCode"] = aLanguageCode;
		res["entry_count"] = entry_count;
		res["oldlog"] = buffer;

		fprintf(fplog, "[%10.3f] (start) rc: %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);
		fprintf(fplog, "[%10.3f] GameId - %s\n", tstamp(), aGameId); fflush(fplog);
		fprintf(fplog, "[%10.3f] GameSecret - %s\n", tstamp(), aGameSecret); fflush(fplog);
		fprintf(fplog, "[%10.3f] GameVersion - %s\n", tstamp(), aGameVersion); fflush(fplog);
		fprintf(fplog, "[%10.3f] Currency - %s\n", tstamp(), aCurrency); fflush(fplog);
		fprintf(fplog, "[%10.3f] LanguageCode - %s\n", tstamp(), aLanguageCode); fflush(fplog);

		free(aGameId);
		free(aGameSecret);
		free(aGameVersion);
		free(aCurrency);
		free(aLanguageCode);
	} else {
		res["status"] = true;
		res["entry_count"] = entry_count;
	}

	return writer.write(res);
}




/*
 * Exposed functions
 *
 */

// *** Get the current users ***

std::string TemplateNDK::getUser() {
	Json::FastWriter writer;
	Json::Value res;
	SC_Error_t rc;

	rc = RequestUser(&app);

	fprintf(fplog, "[%10.3f] (getUser) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;
	return writer.write(res);
}

void TemplateNDK::getUserCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;
	char cash[64];

	sprintf(cash, "%lu", app->cash);

	res["coins"] = cash;
	res["login"] = app->UserInfo->login;
	res["image"] = app->UserInfo->imgurl;

	fprintf(fplog, "[%10.3f] (getUserCallback) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	std::string event = "community.scoreloop.getUserCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}

std::string TemplateNDK::setScore(const std::string& arg) {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;
	SC_Error_t rc = SBSL_BAD_PARAM;
	double aScore;
	double aMinorScore;
	unsigned int aLevel;
	unsigned int aMode;


	bool parse = reader.parse(arg, root);

	if (parse) {
		aScore = atof(root["majorScore"].asCString());
		aMinorScore = atof(root["minorScore"].asCString());
		aLevel = atoi(root["level"].asCString());
		aMode = atoi(root["mode"].asCString());
		rc = SubmitScore(&app, aScore, aMinorScore, aLevel, aMode);
	}


	fprintf(fplog, "[%10.3f] (setScore) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	return writer.write(res);
}

void TemplateNDK::setScoreCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;

	fprintf(fplog, "[%10.3f] (setScoreCallback) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	std::string event = "community.scoreloop.setScoreCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}

std::string TemplateNDK::setChallenge(const std::string& arg) {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;
	SC_Error_t rc = SBSL_BAD_PARAM;
	double sScore;
	double sMinorScore;
	unsigned int sMode;
	unsigned int sLevel;
	unsigned int sWager;
	SC_User_h against = NULL;

	bool parse = reader.parse(arg, root);

	if (parse) {
		sScore = atof(root["majorScore"].asCString());
		sMinorScore = atof(root["minorScore"].asCString());
		sMode = atoi(root["mode"].asCString());
		sLevel = atoi(root["level"].asCString());
		sWager = atoi(root["wager"].asCString());

		rc = CreateChallenge(&app, sWager, sScore, sMinorScore, sMode, sLevel, against);
	}

	fprintf(fplog, "[%10.3f] (setChallenge) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	return writer.write(res);
}

void TemplateNDK::setChallengeCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;

	fprintf(fplog, "[%10.3f] (setChallengeCallback) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	std::string event = "community.scoreloop.setChallengeCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}

std::string TemplateNDK::setChallengeScore(const std::string& arg) {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;
	SC_Error_t rc = SBSL_BAD_PARAM;
	double aScore;
	double aMinorScore;
	unsigned int aLevel;
	unsigned int aMode;


	bool parse = reader.parse(arg, root);

	if (parse) {
		aScore = atof(root["majorScore"].asCString());
		aMinorScore = atof(root["minorScore"].asCString());
		aLevel = atoi(root["level"].asCString());
		aMode = atoi(root["mode"].asCString());
		rc = CreateChallengeScore(&app, aScore, aMinorScore, aLevel, aMode);
	}


	fprintf(fplog, "[%10.3f] (setChallengeScore) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	return writer.write(res);
}

void TemplateNDK::setChallengeScoreCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;

	fprintf(fplog, "[SLTest] (setChallengeScoreCallback)\n"); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	std::string event = "community.scoreloop.setChallengeScoreCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}

std::string TemplateNDK::getChallengeList() {
	Json::FastWriter writer;
	Json::Value res;
	SC_Error_t rc;

	rc = LoadChallenges(&app);

	fprintf(fplog, "[%10.3f] (getChallengeList) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;
	return writer.write(res);
}

/*
static void GetScore(SC_Score score, Json::Value* root) {
	root["result"] = SC_Score_GetResult(score);
	root["minorResult"] = SC_Score_GetMinorResult(score);
	root["level"] = SC_Score_GetLevel(score);
	root["mode"] = SC_Score_GetMode(score);
}
*/

void TemplateNDK::getChallengeListCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;
	unsigned int i;

	for(i = 0; i< app->challenges_c; i++) {
		res["challenges"][i]["login"] = app->challenges[i]->contenderlogin;
		res["challenges"][i]["majorScore"] = SC_Score_GetResult(app->challenges[i]->contenderscore);
		res["challenges"][i]["minorScore"] = SC_Score_GetMinorResult(app->challenges[i]->contenderscore);
		res["challenges"][i]["level"] = SC_Score_GetLevel(app->challenges[i]->contenderscore);
		res["challenges"][i]["mode"] = SC_Score_GetMode(app->challenges[i]->contenderscore);
		res["challenges"][i]["stake"] = SC_Money_GetAmount(app->challenges[i]->prize);
		res["challenges"][i]["prize"] = SC_Money_GetAmount(app->challenges[i]->prize);
		res["challenges"][i]["created"] = app->challenges[i]->created;
	}

	res["challenge_c"] = app->challenges_c;

	fprintf(fplog, "[%10.3f] (getChallengeListCallback) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	std::string event = "community.scoreloop.getChallengeListCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}


std::string TemplateNDK::getLeaders(const std::string& arg) {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;
	SC_Error_t rc = SBSL_BAD_PARAM;
	unsigned int sMode;
	unsigned int sList;
	unsigned int rLen;
	SC_ScoresSearchList_t searchList = SC_SCORES_SEARCH_LIST_ALL;

	bool parse = reader.parse(arg, root);

	if (parse) {
		sMode = atoi(root["searchMode"].asCString());
		sList = atoi(root["searchList"].asCString());
		rLen = atoi(root["range"].asCString());

 		switch (sList) {
		case 1:
			searchList = SC_SCORES_SEARCH_LIST_24H;
			break;
		case 2:
			searchList = SC_SCORES_SEARCH_LIST_USER_COUNTRY;
			break;
		}
		rc = LoadLeaderboard(&app, sMode, searchList, rLen);
	}

	fprintf(fplog, "[%10.3f] (getLeaders) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	return writer.write(res);
}

void TemplateNDK::getLeadersCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;
	unsigned int i;

	for(i = 0; i< app->leaders_c; i++) {
		res["leaders"][i]["login"] = app->leaders[i]->login;
		res["leaders"][i]["rank"] = app->leaders[i]->rank;
		res["leaders"][i]["majorScore"] = app->leaders[i]->majorScore;
		res["leaders"][i]["minorScore"] = app->leaders[i]->minorScore;
		res["leaders"][i]["level"] = app->leaders[i]->level;
		res["leaders"][i]["mode"] = app->leaders[i]->mode;
	}

	fprintf(fplog, "[%10.3f] (getLeadersCallback) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	std::string event = "community.scoreloop.getLeadersCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}







std::string TemplateNDK::getBuddyList() {
	Json::FastWriter writer;
	Json::Value res;
	SC_Error_t rc;

	rc = scgetbuddies(&app);

	fprintf(fplog, "[%10.3f] (getBuddyList) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;
	return writer.write(res);
}

void TemplateNDK::getBuddyListCallback(AppData_t *app, SC_Error_t rc) {
	Json::FastWriter writer;
	Json::Value res;
	unsigned int i;

	for(i = 0; i< app->buddies_c; i++) {
		res["buddylist"][i]["login"] = app->buddies[i]->login;
		res["buddylist"][i]["image"] = app->buddies[i]->imgurl;
	}

	fprintf(fplog, "[%10.3f] (getBuddyListCallback) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;


	std::string event = "community.scoreloop.getBuddyListCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(res));
}




/*
 * Users functions
 *
 */

void usersControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;
    SC_User_h buddy;
    UserInfo_t **buddylist = NULL;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_UsersController_Release(app->usersController); /* Cleanup Controller */
		return;
	}

	SC_UserList_h buddies = SC_UsersController_GetUsers(app->usersController);

	if(buddies) {
		unsigned int ulist = SC_UserList_GetCount(buddies);
		buddylist = (UserInfo_t **) calloc(sizeof(UserInfo_t *), ulist);
		for(unsigned int i=0; i< ulist; i++) {
			buddy = SC_UserList_GetAt(buddies, i);
			buddylist[i] = GetUserInfo(buddy, true);
		}

		app->buddies_c = ulist;
		app->buddies = buddylist;
	}

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getBuddyListCallback(app, completionStatus);

	SC_UsersController_Release(app->usersController); /* Cleanup Controller */
}

void freebuddies(AppData_t app) {
	for(unsigned int i=0; i< app.buddies_c; i++) {
		free((void *) app.buddies[i]->login);
		free((void *) app.buddies[i]->email);
		free((void *) app.buddies[i]->imgurl);
	}
}

SC_Error_t scgetbuddies(AppData_t *app) {
	SC_Error_t rc = SBSL_BAD_PARAM;

	//create user controller
	if((app->client != NULL) && (app->UserInfo != NULL) && (app->UserInfo->user != NULL)) {
		if((rc = SC_Client_CreateUsersController(app->client, &app->usersController, usersControllerCallback, app)) == SC_OK) {
			/* Make the asynchronous request & clear rc's error */
			rc = SC_UsersController_LoadBuddies(app->usersController, app->UserInfo->user);
		}
	}

	if((rc != SC_OK) && (app->usersController != NULL)) {
		SC_UsersController_Release(app->usersController);
	}

	return rc;
}

void userControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_UserController_Release(app->userController); /* Cleanup Controller */
		return;
	}

	/* Get the session from the client. */
	SC_Session_h session = SC_Client_GetSession(app->client);

	/* Get the session user from the session. */
	SC_User_h user = SC_Session_GetUser(session);

	app->UserInfo = GetUserInfo(user, false);

	SC_Money_h money_h = SC_Session_GetBalance(session);
	unsigned long cash = SC_Money_GetAmount(money_h);

	app->cash = cash;

	/* We don't need the UserController anymore, so release it */
	SC_UserController_Release(app->userController);

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getUserCallback(app, completionStatus);

}


// Thread functions
// The following functions are for controlling the main Scoreloop Thread

// The actual thread (must appear before the startThread method)
// Loops and runs the Scoreloop event loop

void* TemplateThread(void* parent) {
	TemplateNDK *pParent = static_cast<TemplateNDK *>(parent);

	// Loop calls the callback function and continues until stop is set
	while (!pParent->isThreadHalt()) {
		// Loop calls Scoreloop and continues until stop is set
		SC_HandleCustomEvent(&initData, SC_FALSE);

		usleep(10);
	}

	return NULL;
}

// Starts the thread and returns a message on status
std::string TemplateNDK::templateStartThread() {
	if (!m_thread) {
		int rc;
	    rc = pthread_mutex_lock(&mutex);
	    threadHalt = false;
	    rc = pthread_cond_signal(&cond);
	    rc = pthread_mutex_unlock(&mutex);

		pthread_attr_t thread_attr;
		pthread_attr_init(&thread_attr);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

		pthread_create(&m_thread, &thread_attr, TemplateThread,
				static_cast<void *>(this));
		pthread_attr_destroy(&thread_attr);
		return "Thread Started";
	} else {
		return "Thread Running";
	}
}

// Sets the stop value
std::string TemplateNDK::templateStopThread() {
	int rc;
	// Request thread to set prevent sleep to false and terminate
	rc = pthread_mutex_lock(&mutex);
	threadHalt = true;
	rc = pthread_cond_signal(&cond);
	rc = pthread_mutex_unlock(&mutex);

    // Wait for the thread to terminate.
    void *exit_status;
    rc = pthread_join(m_thread, &exit_status) ;

	// Clean conditional variable and mutex
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);

	m_thread = 0;
	threadHalt = true;

	// Tidy Up
	SC_User_Release(app.UserInfo->user);
	SC_Client_Release(app.client);

	return "Thread stopped";
}

// getter for the stop value
bool TemplateNDK::isThreadHalt() {
	int rc;
	bool isThreadHalt;
	rc = pthread_mutex_lock(&mutex);
	isThreadHalt = threadHalt;
	rc = pthread_mutex_unlock(&mutex);
	return isThreadHalt;
}



/*-------------------------------------------------------------------------------------*
 * Scoreloop Functions
 *-------------------------------------------------------------------------------------*/

LeaderInfo_t* GetLeaderInfo(SC_User_h user, SC_Score_h oScore) {
	LeaderInfo_t *leader;
	if(NULL != (leader = (LeaderInfo_t *) calloc(1, sizeof(LeaderInfo_t)))) {
		leader->user = SC_Score_GetUser(oScore);
		leader->login = strdup(SC_String_GetData(SC_User_GetLogin(user)));
		leader->majorScore = SC_Score_GetResult(oScore);
		leader->minorScore = SC_Score_GetMinorResult(oScore);
		leader->mode = SC_Score_GetMode(oScore);
		leader->level = SC_Score_GetLevel(oScore);
		leader->rank = SC_Score_GetRank(oScore);
	}
	return leader;
}

UserInfo_t *GetUserInfo(SC_User_h user, bool isBuddy) {
	UserInfo_t *uinfo;
	if(NULL != (uinfo = (UserInfo_t *) calloc(1, sizeof(UserInfo_t)))) {
		/* Get user data */
		uinfo->user = user;
		uinfo->login = strdup(SC_String_GetData(SC_User_GetLogin(user)));
		if(isBuddy) { // Buddy doesn't have access to email
			uinfo->email = NULL;
		} else {
			uinfo->email = strdup(SC_String_GetData(SC_User_GetEmail(user)));
		}
		uinfo->imgurl = strdup(SC_String_GetData(SC_User_GetImageUrl(user)));
		uinfo->buddy_c = SC_User_GetBuddiesCount(user);
		uinfo->games_c = SC_User_GetGamesCount(user);
		uinfo->achievements_c = SC_User_GetGlobalAchievementsCount(user);
		uinfo->challenge = SC_User_IsChallengable(user);
		uinfo->state = SC_User_GetState(user);
		uinfo->handle = SC_User_GetHandle(user);
		uinfo->ctx = SC_User_GetContext(user);
		}

	return uinfo;
}


// Remove %HH from strings
char* urldecode(const char *str) {
	char *ns;
	int l;
	int i;
	int k = 0;
	unsigned long int hexcode;

	l = strlen(str);
	ns = (char*) malloc(sizeof(char) * l);
	memset(ns, 0, l);

	// Convert %hh encoded data
	for(i = 0; i < l; i++) {
		if((i <= (l - 3)) && (str[i] == '%') && ISXDIGIT(str[i + 1]) && ISXDIGIT(str[i + 2])) {
			char hexstr[3];
			char *ptr;
			hexstr[0] = str[i + 1];
			hexstr[1] = str[i + 2];
			hexstr[2] = 0;
			hexcode = strtoul(hexstr, &ptr, 16);
			hexcode = (hexcode & (unsigned long) 0xFF);
			if((hexcode != 34) && (i > 0) && (i < (l - 3))) { // Kill leading or trailing double quotes
				ns[k] = (unsigned char) hexcode;
				k++;
			}
			i += 2;
		} else {
			ns[k] = str[i];
			k++;
		}

	}

	return ns;
}



/*-------------------------------------------------------------------------------------*
 * Scoreloop Functions
 *-------------------------------------------------------------------------------------*/

SC_Error_t RequestUser(AppData_t *app)
{
    /* Create a UserController */
    SC_Error_t rc = SC_Client_CreateUserController(app->client, &app->userController, RequestUserCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Make the asynchronous request */
    rc = SC_UserController_LoadUser(app->userController);
    if (rc != SC_OK) {
        SC_UserController_Release(app->userController);
        return rc;
    }

    return rc;
}

static void RequestUserCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;
    char buf[0x100];

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_UserController_Release(app->userController); /* Cleanup Controller */
        return;
    }

    /* Get the session from the client. */
    SC_Session_h session = SC_Client_GetSession(app->client);

    /* Get the session user from the session. */
    SC_User_h user = SC_Session_GetUser(session);


	app->UserInfo = GetUserInfo(user, false);

	SC_Money_h money_h = SC_Session_GetBalance(session);
	unsigned long cash = SC_Money_GetAmount(money_h);

	app->cash = cash;

	/* We don't need the UserController anymore, so release it */
	SC_UserController_Release(app->userController);

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getUserCallback(app, completionStatus);

}

SC_Error_t SubmitScore(AppData_t *app, double result, double minorResult, unsigned int level, unsigned int mode)
{
    /* Create a ScoreController */
    SC_Error_t rc = SC_Client_CreateScoreController(app->client, &app->scoreController, SubmitScoreCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Create and configure the score */
    rc = SC_Client_CreateScore(app->client, &app->score);
    if (rc != SC_OK) {
        SC_ScoreController_Release(app->scoreController); /* Cleanup Controller */
        return rc;
    }
    SC_Score_SetResult(app->score, result);
	SC_Score_SetMinorResult(app->score, minorResult);
	SC_Score_SetLevel(app->score, level);
	SC_Score_SetMode(app->score, mode);

    /* Submit the score */
    rc = SC_ScoreController_SubmitScore(app->scoreController, app->score);
    if (rc != SC_OK) {
        SC_ScoreController_Release(app->scoreController); /* Cleanup Controller */
        SC_Score_Release(app->score); /* Cleanup Score */
        return rc;
    }

    return rc;
}

static void SubmitScoreCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_ScoreController_Release(app->scoreController); /* Cleanup Controller */
        SC_Score_Release(app->score); /* Cleanup Score */
        return;
    }

    /* Cleanup Controller */
    SC_ScoreController_Release(app->scoreController);
    SC_Score_Release(app->score);

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->setScoreCallback(app, completionStatus);
}

SC_Error_t LoadLeaderboard(AppData_t *app, unsigned int mode, const SC_ScoresSearchList_t searchList, unsigned int count)
{
    /* Create a ScoresController */
    SC_Error_t rc = SC_Client_CreateScoresController(app->client, &app->scoresController, LoadLeaderboardCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Configure the Controller */
    SC_ScoresController_SetMode(app->scoresController, mode);
    rc = SC_ScoresController_SetSearchList(app->scoresController, searchList);
    if (rc != SC_OK) {
        SC_ScoresController_Release(app->scoresController); /* Cleanup Controller */
        return rc;
    }

    /* Load the Leaderboard for the given score and count */
    rc = SC_ScoresController_LoadScoresAroundUser(app->scoresController, app->UserInfo->user, count);
    if (rc != SC_OK) {
        SC_ScoresController_Release(app->scoresController); /* Cleanup Controller */
        return rc;
    }

    return rc;
}

static void LoadLeaderboardCompletionCallback(void *userData, SC_Error_t completionStatus)
{
	SC_ScoreList_h slist;
	SC_Score_h oScore;
	unsigned int scount;
	LeaderInfo_t **leaderlist = NULL;;

    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_ScoresController_Release(app->scoresController); /* Cleanup Controller */
		return;
	}

	if(app->leaders != NULL) {
		for(unsigned int i=0; i< app->leaders_c; i++) {
			free((void *) app->leaders[i]->login);
		}
		free(app->leaders);
		app->leaders_c = 0;
		app->leaders = NULL;
	}

    slist = SC_ScoresController_GetScores(app->scoresController);
    scount = SC_ScoreList_GetCount(slist);

    leaderlist = (LeaderInfo_t **) calloc(sizeof(LeaderInfo_t *), scount);
 	for(unsigned int i=0; i< scount; i++) {
		oScore = SC_ScoreList_GetAt(slist, i);
		SC_User_h user = SC_Score_GetUser(oScore);
		leaderlist[i] = GetLeaderInfo(user, oScore);
	}

	app->leaders_c = scount;
	app->leaders = leaderlist;

    /* Cleanup Controller */
    SC_ScoresController_Release(app->scoresController);

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getLeadersCallback(app, completionStatus);

}

SC_Error_t AchieveAward(AppData_t *app, const char *awardIdentifier)
{
    SC_Bool_t achieved;

    /* Create an Achievements Controller */
    SC_Error_t rc = SC_Client_CreateLocalAchievementsController(app->client, &app->achievementsController, AchieveAwardCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Set the award with the given identifier to be achieved */
    rc = SC_LocalAchievementsController_SetAchievedValueForAwardIdentifier(app->achievementsController, awardIdentifier, &achieved);
    if (rc != SC_OK) {
        SC_LocalAchievementsController_Release(app->achievementsController); /* Cleanup Controller */
        return rc;
    }

    /* Synchronize achievement if indicated - this can be done at some other point in time and does not have to come
     * after every setting of an achievement.
     */
    if (SC_LocalAchievementsController_ShouldSynchronize(app->achievementsController) == SC_TRUE) {
        rc = SC_LocalAchievementsController_Synchronize(app->achievementsController);
        if (rc != SC_OK) {
            SC_LocalAchievementsController_Release(app->achievementsController); /* Cleanup Controller */
            return rc;
        }
    }
    else {
        /* Cleanup Controller */
        SC_LocalAchievementsController_Release(app->achievementsController);

        /* Load Achievement here just for demonstration purposes */
        LoadAchievements(app);
    }

    return rc;
}

static void AchieveAwardCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_LocalAchievementsController_Release(app->achievementsController); /* Cleanup Controller */
        return;
    }

    /* Cleanup Controller */
    SC_LocalAchievementsController_Release(app->achievementsController);

    /* Load Achievement here just for demonstration purposes */
    LoadAchievements(app);
}

SC_Error_t LoadAchievements(AppData_t *app)
{
    /* Create an Achievements Controller */
    SC_Error_t rc = SC_Client_CreateLocalAchievementsController(app->client, &app->achievementsController, LoadAchievementsCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Load the achievements */
    LoadAchievementsCompletionCallback(app, SC_OK);

    return rc;
}

static void LoadAchievementsCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_LocalAchievementsController_Release(app->achievementsController); /* Cleanup Controller */
        return;
    }

    /* Just log the achievements here for demonstration purposes */
    SC_AchievementList_h achievementList = SC_LocalAchievementsController_GetAchievements(app->achievementsController);
    if (achievementList == NULL) {
        SC_LocalAchievementsController_Release(app->achievementsController); /* Cleanup Controller */
        return;
    }

    unsigned int i, numAchievements = SC_AchievementList_GetCount(achievementList);
    for (i = 0; i < numAchievements; ++i) {
        SC_Achievement_h achievement = SC_AchievementList_GetAt(achievementList, i);
        SC_Award_h award = SC_Achievement_GetAward(achievement);
    }

    /* Cleanup Controller */
    SC_LocalAchievementsController_Release(app->achievementsController);

}

SC_Error_t CreateChallenge(AppData_t *app, unsigned int amount, double result, double minorResult, unsigned int mode, unsigned int level, SC_User_h against)
{
    SC_Error_t rc;
    SC_Challenge_h challenge;
    SC_Money_h stake;
	SC_Score_h score;

    stake = CreateMoney(app, amount);

    /* Create a ChallengeController */
    rc = SC_Client_CreateChallengeController(app->client, &app->challengeController, CreateChallengeCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Create Challenge */
    rc = SC_Client_CreateChallenge(app->client, stake, against, mode, level, &challenge);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    rc = SC_ChallengeController_SetChallenge(app->challengeController, challenge);
    SC_Challenge_Release(challenge); /* Not needed anymore, so cleanup */
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

	/* Create a Score object with given result */
    rc = SC_Client_CreateScore(app->client, &score);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    if((rc = SC_Score_SetResult(score, result)) == SC_OK) {
    	if((rc = SC_Score_SetMinorResult(score, minorResult)) == SC_OK) {
    		if((rc = SC_Score_SetLevel(score, level)) == SC_OK) {
    			rc = SC_Score_SetMode(score, mode);
    		}
    	}
    }
	if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    rc = SC_ChallengeController_SubmitChallengeScore(app->challengeController, score);
    SC_Score_Release(score); /* Not needed anymore, so cleanup */
    if (rc != SC_OK) {
    	fprintf(fplog, "[%10.3f] (CreateChallenge) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    return rc;
}


SC_Error_t CreateChallengeScore(AppData_t *app, double result, double minorResult, unsigned int level, unsigned int mode)
{
	SC_Error_t rc;
	SC_Score_h score;

    /* Create a ChallengeController */
    rc = SC_Client_CreateChallengeController(app->client, &app->challengeController, CreateChallengeScoreCallback, app);
    if (rc != SC_OK) {
        SC_Challenge_Release(app->challenge);
        return rc;
    }

	/* Create a Score object with given result */
    rc = SC_Client_CreateScore(app->client, &score);
    if (rc != SC_OK) {
        SC_Challenge_Release(app->challenge);
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    if((rc = SC_Score_SetResult(score, result)) == SC_OK) {
		SC_Score_SetMinorResult(app->score, minorResult);
		SC_Score_SetLevel(app->score, level);
		SC_Score_SetMode(app->score, mode);
    }

	if (rc != SC_OK) {
	    SC_Challenge_Release(app->challenge);
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        SC_Score_Release(score); /* Cleanup Score */
        return rc;
    }

    /* Submit Challenge for score */
	fprintf(fplog, "[%10.3f] (CreateChallengeScore) - Submit time\n", tstamp()); fflush(fplog);

    rc = SC_ChallengeController_SubmitChallengeScore(app->challengeController, score);
    SC_Score_Release(score); /* Not needed anymore, so cleanup */
    if (rc != SC_OK) {
    	fprintf(fplog, "[%10.3f] (CreateChallengeScore) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);
	    SC_Challenge_Release(app->challenge);
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    } else {
    	fprintf(fplog, "[%10.3f] (CreateChallengeScore) - %d (%s)\n", tstamp(), rc, SC_MapErrorToStr(rc)); fflush(fplog);
    }

    return rc;
}

static void CreateChallengeCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return;
    }

    /* Cleanup Controller */
    SC_Challenge_Retain(app->challenge);
    SC_ChallengeController_Release(app->challengeController);
    SC_Challenge_Release(app->challenge);

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
    ndk->setChallengeCallback(app, completionStatus);

}

SC_Error_t LoadChallenges(AppData_t *app)
{
    /* Create a Challenges Controller */
    SC_Error_t rc = SC_Client_CreateChallengesController(app->client, &app->challengesController, LoadChallengesCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Load the open challenges */
    rc = SC_ChallengesController_LoadOpenChallenges(app->challengesController);
    if (rc != SC_OK) {
        SC_ChallengesController_Release(app->challengesController); /* Cleanup Controller */
        return rc;
    }

    return rc;
}

ChallengeInfo_t* GetChallengeInfo(SC_Challenge_h oChallenge) {
	ChallengeInfo_t *challenge;
	SC_User_h user;
	if(NULL != (challenge = (ChallengeInfo_t *) calloc(1, sizeof(ChallengeInfo_t)))) {
		if((user = SC_Challenge_GetContender(oChallenge)) != NULL) {
			challenge->contender = user;
			challenge->contenderlogin = strdup(SC_String_GetData(SC_User_GetLogin(user)));
			challenge->contenderscore = SC_Challenge_GetContenderScore(oChallenge);
		}

		if((user = SC_Challenge_GetContestant(oChallenge)) != NULL) {
			challenge->contestant = user;
			challenge->contestantlogin = strdup(SC_String_GetData(SC_User_GetLogin(user)));
			challenge->contestantscore = SC_Challenge_GetContestantScore(oChallenge);
		}

		challenge->created = strdup(SC_String_GetData(SC_Challenge_GetCreatedAt(oChallenge)));

		challenge->stake = SC_Challenge_GetStake(oChallenge);
		challenge->mode = SC_Challenge_GetMode(oChallenge);
		challenge->level = SC_Challenge_GetLevel(oChallenge);
		challenge->prize = SC_Challenge_GetPrize(oChallenge);
	}
	return challenge;
}

static void LoadChallengesCompletionCallback(void *userData, SC_Error_t completionStatus)
{
	SC_ChallengeList_h clist;
	SC_Challenge_h oChallenge;
	unsigned int ccount, i;
	ChallengeInfo_t **challengelist;

    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_ChallengesController_Release(app->challengesController); /* Cleanup Controller */
		return;
	}

	if(app->challenges != NULL) {
		for(i=0; i<app->challenges_c; i++) {
			free((void *) app->challenges[i]->contenderlogin);
			free((void *) app->challenges[i]->contestantlogin);
			free((void *) app->challenges[i]->created);
			}
		free(app->challenges);
		app->challenges_c = 0;
	}

    clist = SC_ChallengesController_GetChallenges(app->challengesController);
    ccount = SC_ChallengeList_GetCount(clist);

    challengelist = (ChallengeInfo_t **) calloc(sizeof(ChallengeInfo_t *), ccount);
 	for(i=0; i< ccount; i++) {
		oChallenge = SC_ChallengeList_GetAt(clist, i);
		challengelist[i] = GetChallengeInfo(oChallenge);
	}

 	app->challenges_c = ccount;
	app->challenges = challengelist;

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getChallengeListCallback(app, completionStatus);

    /* Cleanup Controller */
    SC_ChallengesController_Release(app->challengesController);
}

SC_Error_t AcceptChallenge(AppData_t *app, SC_Challenge_h challenge)
{
    /* Create a ChallengeController */
    SC_Error_t rc = SC_Client_CreateChallengeController(app->client, &app->challengeController, AcceptChallengeCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Set challenge to be accepted */
    rc = SC_ChallengeController_SetChallenge(app->challengeController, challenge);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    /* Inform server that we accepted that challenge */
    rc = SC_ChallengeController_AcceptChallenge(app->challengeController);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    return rc;
}

static void AcceptChallengeCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;
    SC_Challenge_h challenge;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return;
    }

    /* In your game you would now start the game play to get a score result. Here we just
     * pick a score result and submit the challenge
     */
    challenge = SC_ChallengeController_GetChallenge(app->challengeController);

    /* Cleanup Controller - In order to have challenge stay alive, bump its retain-count */
    SC_Challenge_Retain(challenge);
    SC_ChallengeController_Release(app->challengeController);

    /* Decrement retain count on challenge */
    SC_Challenge_Release(challenge);
}

SC_Error_t SubmitChallenge(AppData_t *app, SC_Challenge_h challenge, double result, unsigned int mode)
{
    SC_Error_t rc;
    SC_Score_h score;

    /* Create a ChallengeController */
    rc = SC_Client_CreateChallengeController(app->client, &app->challengeController, SubmitChallengeCompletionCallback, app);
    if (rc != SC_OK) {
        return rc;
    }

    /* Set challenge to be accepted */
    rc = SC_ChallengeController_SetChallenge(app->challengeController, challenge);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    /* Create a Score object with given result and mode */
    rc = SC_Client_CreateScore(app->client, &score);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }
    rc = SC_Score_SetResult(score, result);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        SC_Score_Release(score); /* Cleanup Score */
        return rc;
    }
    rc = SC_Score_SetMode(score, mode);
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        SC_Score_Release(score); /* Cleanup Score */
        return rc;
    }

    /* Submit Challenge for score */
    rc = SC_ChallengeController_SubmitChallengeScore(app->challengeController, score);
    SC_Score_Release(score); /* Not needed anymore, so cleanup */
    if (rc != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return rc;
    }

    return rc;
}

static void SubmitChallengeCompletionCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return;
    }

    /* Cleanup Controller */
    SC_ChallengeController_Release(app->challengeController);

    /* Done with sample, so just inform user for demonstration purposes */
}

static void CreateChallengeScoreCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    /* Check completion status */
    if (completionStatus != SC_OK) {
        SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
        return;
    }

    /* Cleanup Controller */
    SC_Challenge_Release(app->challenge);
    SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->setChallengeScoreCallback(app, completionStatus);

    /* Done with sample, so just inform user for demonstration purposes */
}

static SC_Money_h CreateMoney(AppData_t *app, unsigned int amount) {
	SC_Money_h money = NULL;

	if(SC_Client_CreateMoney(app->client, &money, amount) != SC_OK) {
		money = NULL;
	}

	return money;
}




} /* namespace webworks */
