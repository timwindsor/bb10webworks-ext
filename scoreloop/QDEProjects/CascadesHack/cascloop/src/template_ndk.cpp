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
#include "json/writer.h"
#include "json/reader.h"
#include <pthread.h>
#include "template_ndk.hpp"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <scoreloop/scoreloopcore.h>
#include <pthread.h>

#include <unistd.h>

#include <QDebug>

/* Some simple logging */
// #define LOG(fmt, args...)   do { fprintf(stdout, "[Scoreloop Test] " fmt "\n", ##args); fflush(stdout); } while (0);

AppData_t app;

SC_InitData_t initData;
bool threadHalt;
pthread_t m_thread;
pthread_cond_t cond;
pthread_mutex_t mutex;

void init() {
	m_thread = 0;
	pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	threadHalt = true;

	// Initialize the platform adaptation object
	SC_InitData_Init(&initData);

	// Optionally modify the following fields:
	// initData.currentVersion = SC_INIT_CURRENT_VERSION;
	// initData.minimumRequiredVersion = SC_INIT_VERSION_1_0;
	// initData.runLoopType = SC_RUN_LOOP_TYPE_BPS;

}


void kill() {
	if(!threadHalt) {
		templateStopThread();
	}
}

SC_Error_t userget() {
	SC_Error_t rc;

	if((rc = scgetuser(&app)) == SC_OK) {
		}

	return rc;
}


// These methods are the true native code we intend to reach from WebWorks

SC_Error_t start() {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;
	SC_Error_t rc;

    memset(&app, 0, sizeof(AppData_t));

	char *aGameId = (char *) "cdc1ee7e-403b-4d68-ac1a-cb1ebf36f782";
	char *aGameSecret = (char *) "dNVkPkrrp68BOeOu4NqUx1ZeF+iWY21BEFF4hT4A7spAuIYUhiE49w==";
	char *aGameVersion = (char *) "1.0";
	char *aCurrency = (char *) "ACF";
	char *aLanguageCode = (char *) "en";

	// aGameId, aGameSecret and aCurrency are const char strings that you obtain from Scoreloop.
	// aGameVersion should be your current game version.
	// aLanguageCode specifies the language support for localization in awards,
	// for example, "en" for English, which is the default language.

	if(threadHalt) {
		initData.runLoopType = SC_RUN_LOOP_TYPE_CUSTOM;

		rc = SC_Client_New(&app.client, &initData, aGameId, aGameSecret, aGameVersion, aCurrency, aLanguageCode);

		if(rc == SC_OK) {
			templateStartThread();
		}
	}


	return rc;
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


LeaderInfo_t *GetLeaderInfo(SC_User_h user, SC_Score_h oScore) {
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

void usersControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;
    SC_User_h buddy;
    UserInfo_t *uinfo;
    void **buddylist = NULL;
	SC_Error_t rc;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_UsersController_Release(app->usersController); /* Cleanup Controller */
		app->usersController = NULL;
		return;
	}

	SC_UserList_h buddies = SC_UsersController_GetUsers(app->usersController); // SBHack

	if(buddies) {
		unsigned int ulist = SC_UserList_GetCount(buddies);
		UserInfo_t **buddylist = (UserInfo_t **) calloc(sizeof(UserInfo_t *), ulist);
		for(unsigned int i=0; i< ulist; i++) {
			buddy = SC_UserList_GetAt(buddies, i);
			buddylist[i] = GetUserInfo(buddy, true);
//	        LOG("User: %s", buddylist[i]->login);
	        qDebug() << "User:  " << buddylist[i]->login;
		}

		app->buddies_c = ulist;
		app->buddies = buddylist;
	}

	srand(time(NULL));
	unsigned int aMode = rand() % 10; // SBHack
    scgetscores(app, aMode, SC_SCORES_SEARCH_LIST_ALL, 100); // SBHack

	SC_UsersController_Release(app->usersController);
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
	SC_Error_t rc;

	//create user controller
	if(app->client != NULL) {
		if((rc = SC_Client_CreateUsersController(app->client, &app->usersController, usersControllerCallback, app)) == SC_OK) {
			/* Make the asynchronous request */
			if((app->UserInfo != NULL) && (app->UserInfo->user != NULL)) {
				if((rc = SC_UsersController_LoadBuddies(app->usersController, app->UserInfo->user)) != SC_OK) {
					SC_UsersController_Release(app->usersController);
					app->usersController = NULL;
				}
			} else {
				SC_UsersController_Release(app->usersController);
				rc = SC_INVALID_STATE;
				app->usersController = NULL;
			}
		}
	} else {
		rc = SC_INVALID_STATE;
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
		app->userController = NULL;
		return;
	}


	/* Get the session from the client. */
	SC_Session_h session = SC_Client_GetSession(app->client);

	/* Get the session user from the session. */
	SC_User_h user = SC_Session_GetUser(session);

	app->UserInfo = GetUserInfo(user, false);

	scgetbuddies(app);

	/* We don't need the UserController anymore, so release it */
	SC_UserController_Release(app->userController);
	app->userController = NULL;
}


SC_Error_t scgetuser(AppData_t *app) {
	SC_Error_t rc;

	//create user controller
	if(app->client != NULL) {
		if((rc = SC_Client_CreateUserController(app->client, &app->userController, userControllerCallback, app)) == SC_OK) {
			/* Make the asynchronous request */
			if((rc = SC_UserController_LoadUser(app->userController)) != SC_OK) {
				SC_UserController_Release(app->userController);
				app->userController = NULL;
			}
		}
	} else {
		rc = SC_INVALID_STATE;
	}
	return rc;
}

void scoreControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

//    scgetscores(app, 0, SC_SCORES_SEARCH_LIST_ALL, 100); // SBHack

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

void rankingControllerCallback(void *userData, SC_Error_t completionStatus)
{
	unsigned int urank;

    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;
    urank = SC_RankingController_GetRanking(app->rankingController);
//    LOG("Rank: %u", urank);
    qDebug() << "Rank: " << urank;

    // Process Rank - watch for SC_SCORE_RANK_OUT_OF_RANGE

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

    // Process Leaderboard

	srand(time(NULL));
	double aScore = rand() % 10001; // SBHack
	double aMinorScore = (rand() % 10001) / 100; // SBHack
	unsigned int aLevel = rand() % 10; // SBHack
	unsigned int aMode = rand() % 10; // SBHack
	scsetscore(app, aScore, &aMinorScore, &aLevel, &aMode); // SBHack
//    LOG("Score: %d, MinorScore: %d, Level: %u, Mode: %u", aScore, aMinorScore, aLevel, aMode);

//	SC_ScoresController_Release(app->scoresController); /* Cleanup Controller */
//	app->scoresController = NULL;
}

SC_Bool_t schasnextrange(AppData_t *app) {
	SC_Bool_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_HasNextRange(app->scoresController);
		app->scoresController = NULL;
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scgetnextrange(AppData_t *app) {
	SC_Error_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_LoadNextRange(app->scoresController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Bool_t schasprevrange(AppData_t *app) {
	SC_Bool_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_HasPreviousRange(app->scoresController);
		app->scoresController = NULL;
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scgetprevrange(AppData_t *app) {
	SC_Error_t  rc;

	if(app->scoresController != NULL) {
		rc = SC_ScoresController_LoadPreviousRange(app->scoresController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scfreescores(AppData_t *app) {
	SC_Error_t rc;

	if(app->scoresController != NULL) {
		rc = SC_OK;
		SC_ScoresController_Release(app->scoresController);
		app->scoresController = NULL;
	} else {
		rc = SC_INVALID_STATE;
	}

	return rc;
}

// searchList = SC_SCORES_SEARCH_LIST_ALL | SC_SCORES_SEARCH_LIST_24H | SC_SCORES_SEARCH_LIST_USER_COUNTRY.
SC_Error_t scgetscores(AppData_t *app, unsigned int sMode, const SC_ScoresSearchList_t searchList, unsigned int rangeLength) {
	SC_Error_t rc;
	//create user controller
	if(app->scoresController != NULL) {
		return SC_INVALID_STATE;
	}
	if(app->client != NULL) {
		if((rc = SC_Client_CreateScoresController(app->client, &app->scoresController, scoresControllerCallback, app)) == SC_OK) {
			if((rc = SC_Client_CreateRankingController(app->client, &app->rankingController, rankingControllerCallback, app)) == SC_OK) {
				if((rc = SC_ScoresController_SetSearchList(app->scoresController, searchList)) == SC_OK) {
					if((app->UserInfo != NULL) && (app->UserInfo->user != NULL)) {
						if((rc = SC_ScoresController_SetMode(app->scoresController, sMode)) == SC_OK) {
							if((rc = SC_RankingController_LoadRankingForUserInMode(app->rankingController, app->UserInfo->user, sMode)) == SC_OK) {		// Triggers rankingControllerCallback
								if((rc = SC_ScoresController_LoadScoresAroundUser(app->scoresController, app->UserInfo->user, rangeLength)) == SC_OK) {	// Triggers scoresControllerCallback
									SC_ScoresController_Retain(app->scoresController);
								}
							}
						}
					}
				}
				if(rc != SC_OK) {
					SC_RankingController_Release(app->rankingController);
					SC_ScoresController_Release(app->scoresController);
					app->scoresController = NULL;
					app->rankingController = NULL;
				}
			} else {
				SC_ScoresController_Release(app->scoresController);
				app->scoresController = NULL;
			}
		}
	} else {
		rc = SC_INVALID_STATE;
	}

	return rc;
}

// Thread functions
// The following functions are for controlling the main Scoreloop Thread

// The actual thread (must appear before the startThread method)
// Loops and runs the Scoreloop event loop

void* TemplateThread(void* unused) {

	// Loop calls Scoreloop and continues until stop is set
	while (!isThreadHalt()) {
		SC_HandleCustomEvent(&initData, SC_FALSE);

		usleep(10);
	}

	return NULL;
}

// Starts the thread and returns a message on status
std::string templateStartThread() {
	if (!m_thread) {
		int rc;
	    rc = pthread_mutex_lock(&mutex);
	    threadHalt = false;
	    rc = pthread_cond_signal(&cond);
	    rc = pthread_mutex_unlock(&mutex);

		pthread_attr_t thread_attr;
		pthread_attr_init(&thread_attr);
		pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);

		pthread_create(&m_thread, &thread_attr, TemplateThread, NULL);
		pthread_attr_destroy(&thread_attr);
		return "Thread Started";
	} else {
		return "Thread Running";
	}
}

// Sets the stop value
std::string templateStopThread() {
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
	if(app.leaders) {
		for(unsigned int i=0; i< app.leaders_c; i++) {
			free((void *) app.leaders[i]->login);
		}
		free(app.leaders);
		app.leaders_c = 0;
	}
	if(app.buddies) {
		freebuddies(app);
		free(app.buddies);
		app.buddies_c = 0;
	}
	SC_User_Release(app.UserInfo->user);
	SC_Client_Release(app.client);

	return "Thread stopped";
}

// The callback method that sends an event through JNEXT
void templateThreadCallback() {
	std::string event = "community.templateExt.jsonThreadCallback";
	Json::FastWriter writer;
	Json::Value root;
}

// getter for the stop value
bool isThreadHalt() {
	int rc;
	bool isThreadHalt;
	rc = pthread_mutex_lock(&mutex);
	isThreadHalt = threadHalt;
	rc = pthread_mutex_unlock(&mutex);
	return isThreadHalt;
}

