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

#define SBSL_BAD_PARAM 32888
#define ISXDIGIT(x) (isxdigit((int) ((unsigned char) x)))

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

namespace webworks {

AppData_t app;
SC_InitData_t initData;
FILE* fp;
char* buffer = NULL;
int entry_count = 0;

SC_Error_t scgetuser(AppData_t *app);
SC_Error_t scgetbuddies(AppData_t *app);

SC_Error_t scsetscore(AppData_t *app, double aScore, double *aMinorScore, unsigned int *aLevel, unsigned int *aMode);
SC_Error_t scgetscores(AppData_t *app, unsigned int sMode, const SC_ScoresSearchList_t searchList, unsigned int rangeLength);
SC_Bool_t scscoreshasprevrange(AppData_t *app);
SC_Bool_t scscoreshasnextrange(AppData_t *app);
SC_Error_t scscoresgetnextrange(AppData_t *app);
SC_Error_t scscoresgetprevrange(AppData_t *app);

TemplateNDK::TemplateNDK(TemplateJS *parent) {
	m_pParent = parent;
	m_thread = 0;
	pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	threadHalt = true;

	// Initialize the platform adaptation object
	SC_InitData_Init(&initData);
    fp = fopen ("data/file.txt", "w+");

	// Optionally modify the following fields:
	// initData.currentVersion = SC_INIT_CURRENT_VERSION;
	// initData.minimumRequiredVersion = SC_INIT_VERSION_1_0;
	// initData.runLoopType = SC_RUN_LOOP_TYPE_BPS;

}


TemplateNDK::~TemplateNDK() {
	if(!threadHalt) {
		templateStopThread();
	}
	fclose(fp);
	if(buffer != NULL) {
		free(buffer);
	}

}

// These methods are the true native code we intend to reach from WebWorks

std::string TemplateNDK::readLog() {
	Json::FastWriter writer;
	Json::Value root;
	long lSize;

	fseek (fp , 0 , SEEK_END);
	lSize = ftell (fp) + 1;
	rewind (fp);

	if(buffer != NULL) {
		free(buffer);
	}
	buffer = (char*) calloc (1, sizeof(char)*lSize);
	fread (buffer,1,lSize,fp);


	fseek (fp , 0 , SEEK_END);

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

		fprintf(fp, "[SLTest] (start) rc: %d\n", rc);
		fprintf(fp, "[SLTest] GameId - %s\n", aGameId);
		fprintf(fp, "[SLTest] GameSecret - %s\n", aGameSecret);
		fprintf(fp, "[SLTest] GameVersion - %s\n", aGameVersion);
		fprintf(fp, "[SLTest] Currency - %s\n", aCurrency);
		fprintf(fp, "[SLTest] LanguageCode - %s\n", aLanguageCode);

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

std::string TemplateNDK::getUser() {
	Json::FastWriter writer;
	Json::Value res;
	SC_Error_t rc;

	rc = scgetuser(&app);

	fprintf(fp, "[SLTest] (getUser)\n");

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;
	return writer.write(res);
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
		rc = scsetscore(&app, aScore, &aMinorScore, &aLevel, &aMode);
	}


	fprintf(fp, "[SLTest] (setScore)\n");

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;
//	return writer.write(root);
	return writer.write(res);
}

std::string TemplateNDK::getBuddyList() {
	Json::FastWriter writer;
	Json::Value res;
	SC_Error_t rc;

	rc = scgetbuddies(&app);

	fprintf(fp, "[SLTest] (getBuddyList)\n");

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;
	return writer.write(res);
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
/*
 		res["searchMode"] = root["searchMode"];
		res["searchList"] = root["searchList"];
		res["range"] = root["range"];

		res["searchMode"] = atoi(root["searchMode"].asCString());
		res["searchList"] = atoi(root["searchList"].asCString());
		res["range"] = atoi(root["range"].asCString());
*/

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
		rc = scgetscores(&app, sMode, searchList, rLen);
	}

	fprintf(fp, "[SLTest] (getLeaders)\n");

	if(rc == SC_OK) {
		res["status"] = true;
	} else {
		res["status"] = false;
	}

	res["rc"] = rc;

	return writer.write(res);
}

/*
 * Callbacks for exposed functions
 *
 */

void TemplateNDK::getUserCallback(AppData_t *app) {
	Json::FastWriter writer;
	Json::Value root;
	char cash[64];

	sprintf(cash, "%lu", app->cash);

	root["coins"] = cash;
	root["login"] = app->UserInfo->login;
	root["image"] = app->UserInfo->imgurl;

	fprintf(fp, "[SLTest] (getUserCallback)\n");

	std::string event = "community.scoreloop.getUserCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(root));
}

void TemplateNDK::getBuddyListCallback(AppData_t *app) {
	Json::FastWriter writer;
	Json::Value root;
	unsigned int i;

	for(i = 0; i< app->buddies_c; i++) {
		root["buddylist"][i]["login"] = app->buddies[i]->login;
		root["buddylist"][i]["image"] = app->buddies[i]->imgurl;
	}

	fprintf(fp, "[SLTest] (getBuddyListCallback)\n");

	std::string event = "community.scoreloop.getBuddyListCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(root));
}

void TemplateNDK::getLeadersCallback(AppData_t *app) {
	Json::FastWriter writer;
	Json::Value root;
	unsigned int i;

	for(i = 0; i< app->leaders_c; i++) {
		root["leaders"][i]["login"] = app->leaders[i]->login;
		root["leaders"][i]["rank"] = app->leaders[i]->rank;
		root["leaders"][i]["majorScore"] = app->leaders[i]->majorScore;
		root["leaders"][i]["minorScore"] = app->leaders[i]->minorScore;
		root["leaders"][i]["level"] = app->leaders[i]->level;
		root["leaders"][i]["mode"] = app->leaders[i]->mode;
	}

	fprintf(fp, "[SLTest] (getLeadersCallback)\n");

	std::string event = "community.scoreloop.getLeadersCallback";
	m_pParent->NotifyEvent(event + " " + writer.write(root));
}




/*
 * Users functions
 *
 */

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

	fprintf(fp, "[SLTest] (getUserInfo)\n");

	return uinfo;
}

void usersControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;
    SC_User_h buddy;
    UserInfo_t **buddylist = NULL;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_UsersController_Release(app->usersController); /* Cleanup Controller */
		app->usersController = NULL;
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
	ndk->getBuddyListCallback(app);

	SC_UsersController_Release(app->usersController); /* Cleanup Controller */
	app->usersController = NULL;
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
		app->usersController = NULL;
	}

	return rc;
}

void userControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

    fprintf(fp, "[SLTest] (getControllerCallback - 1)\n");

	/* Check completion status */
	if (completionStatus != SC_OK) {
	    fprintf(fp, "[SLTest] (getControllerCallback - CS) rc: %d - %s\n", completionStatus, SC_MapErrorToStr(completionStatus));
		SC_UserController_Release(app->userController); /* Cleanup Controller */
		app->userController = NULL;
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

	fprintf(fp, "[SLTest] (getControllerCallback - 2) Cash: %lu\n", cash);
	/* We don't need the UserController anymore, so release it */
	SC_UserController_Release(app->userController);
	app->userController = NULL;

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getUserCallback(app);

	fprintf(fp, "[SLTest] (getControllerCallback - 3)\n");

}


SC_Error_t scgetuser(AppData_t *app) {
	SC_Error_t rc = SBSL_BAD_PARAM;

	//create user controller
	if(app->client != NULL) {
		if((rc = SC_Client_CreateUserController(app->client, &app->userController, userControllerCallback, app)) == SC_OK) {
			/* Make the asynchronous request + reset rc */
			rc = SC_UserController_LoadUser(app->userController);
		}
	}

	if((rc != SC_OK) && (app->userController != NULL)) {
		SC_UserController_Release(app->userController);
		app->userController = NULL;
	}

	fprintf(fp, "[SLTest] (scgetuser) rc: %d\n", rc);

	return rc;
}





/*
 * Scores functions
 *
 */

void scoreControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_ScoreController_Release(app->scoreController); /* Cleanup Controller */
		app->scoreController = NULL;
		return;
	}

	SC_ScoreController_Release(app->scoreController); /* Cleanup Controller */
	app->scoreController = NULL;

}

SC_Error_t scsetscore(AppData_t *app, double aScore, double *aMinorScore = NULL, unsigned int *aLevel = NULL, unsigned int *aMode = NULL) {
	SC_Error_t rc;

	// Create Score object
	if(app->client != NULL) {
		if((rc = SC_Client_CreateScore(app->client, &app->score)) == SC_OK) {

			rc = SC_Score_SetResult(app->score, aScore);

			if(rc == SC_OK) {
				if(aMinorScore != NULL) {
					rc = SC_Score_SetMinorResult(app->score, *aMinorScore);
					if(rc != SC_OK) {
						SC_Score_Release(app->score);
						app->score = NULL;
						return rc;
					}
				}

				if(aLevel != NULL) {
					rc = SC_Score_SetLevel(app->score, *aLevel);
					if(rc != SC_OK) {
						SC_Score_Release(app->score);
						app->score = NULL;
						return rc;
					}
				}

				if(aMode != NULL) {
					rc = SC_Score_SetMode(app->score, *aMode);
					if(rc != SC_OK) {
						SC_Score_Release(app->score);
						app->score = NULL;
						return rc;
					}
				}

				//create score controller
				if((rc = SC_Client_CreateScoreController(app->client, &app->scoreController, scoreControllerCallback, app)) == SC_OK) {
					/* Make the asynchronous request */
					if((rc = SC_ScoreController_SubmitScore(app->scoreController, app->score)) != SC_OK) {
						SC_ScoreController_Release(app->scoreController);
						app->scoreController = NULL;
					}
				}
			}
		}
	} else {
		rc = SC_INVALID_STATE;
	}
	return rc;
}

/*
 * Leaderboard functions
 *
 * Requires call to scfreescores to start an new scgetscores
 *
 */

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

void rankingControllerCallback(void *userData, SC_Error_t completionStatus)
{
	unsigned int urank;

    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_RankingController_Release(app->rankingController); /* Cleanup Controller */
		app->rankingController = NULL;
		return;
	}

    urank = SC_RankingController_GetRanking(app->rankingController);

    // Process Rank - watch for SC_SCORE_RANK_OUT_OF_RANGE

    app->rank = urank;

	SC_RankingController_Release(app->rankingController); /* Cleanup Controller */
	app->rankingController = NULL;
}

void scoresControllerCallback(void *userData, SC_Error_t completionStatus)
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
		app->scoresController = NULL;
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
		fprintf(stdout, "[Scoreloop Test] Leader: %u %f %s\n", leaderlist[i]->rank, leaderlist[i]->majorScore, leaderlist[i]->login); fflush(stdout);
	}

	app->leaders_c = scount;
	app->leaders = leaderlist;

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getLeadersCallback(app);

}

SC_Bool_t scscoreshasnextrange(AppData_t *app) {
	SC_Bool_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_HasNextRange(app->scoresController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scscoresgetnextrange(AppData_t *app) {
	SC_Error_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_LoadNextRange(app->scoresController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Bool_t scscoreshasprevrange(AppData_t *app) {
	SC_Bool_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_HasPreviousRange(app->scoresController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scscoresgetprevrange(AppData_t *app) {
	SC_Error_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_LoadPreviousRange(app->scoresController);
	} else {
		rc = false;
	}

	return rc;
}

// searchList = SC_SCORES_SEARCH_LIST_ALL | SC_SCORES_SEARCH_LIST_24H | SC_SCORES_SEARCH_LIST_USER_COUNTRY.
SC_Error_t scgetscores(AppData_t *app, unsigned int sMode, const SC_ScoresSearchList_t searchList, unsigned int rangeLength) {
	SC_Error_t rc = SC_INVALID_STATE;
	//create user controller
	if(app->scoresController != NULL) {
		SC_RankingController_Release(app->rankingController);
		SC_ScoresController_Release(app->scoresController);
		app->scoresController = NULL;
		app->rankingController = NULL;
	}
	if(app->client != NULL) {
		if((rc = SC_Client_CreateScoresController(app->client, &app->scoresController, scoresControllerCallback, app)) == SC_OK) {
			if((rc = SC_Client_CreateRankingController(app->client, &app->rankingController, rankingControllerCallback, app)) == SC_OK) {
				if((rc = SC_ScoresController_SetSearchList(app->scoresController, searchList)) == SC_OK) {
					if((app->UserInfo != NULL) && (app->UserInfo->user != NULL)) {
						if((rc = SC_ScoresController_SetMode(app->scoresController, sMode)) == SC_OK) {
							if((rc = SC_RankingController_LoadRankingForUserInMode(app->rankingController, app->UserInfo->user, sMode)) == SC_OK) {		// Triggers rankingControllerCallback
								if((rc = SC_ScoresController_LoadScoresAroundUser(app->scoresController, app->UserInfo->user, rangeLength)) == SC_OK) {	// Triggers scoresControllerCallback
								}
							}
						}
					}
				}
			}
		}
	}

	return rc;
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


} /* namespace webworks */
