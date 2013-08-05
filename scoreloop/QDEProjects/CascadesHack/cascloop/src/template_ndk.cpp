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
		rc = scgetgames(&app, SC_GAMES_SEARCH_LIST_ALL, 100);
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

	SC_UserList_h buddies = SC_UsersController_GetUsers(app->usersController); // SBHack

	if(buddies) {
		unsigned int ulist = SC_UserList_GetCount(buddies);
		buddylist = (UserInfo_t **) calloc(sizeof(UserInfo_t *), ulist);
		for(unsigned int i=0; i< ulist; i++) {
			buddy = SC_UserList_GetAt(buddies, i);
			buddylist[i] = GetUserInfo(buddy, true);
			fprintf(stdout, "[Scoreloop Test] Buddy: %s\n", buddylist[i]->login); fflush(stdout);
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
	static int refcount = 0;

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

	SC_Money_h money_h = SC_Session_GetBalance(session);
	unsigned long cash = SC_Money_GetAmount(money_h);

	app->cash = cash;

	scgetbuddies(app);  // SBHack

	refcount++;

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

	unsigned int aLevel = rand() % 10; // SBHack
	unsigned int aMode = rand() % 10; // SBHack
	sccreatechallenge(app, 25, aMode, aLevel, NULL); // SBHack
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

    // Process Leaderboard

	srand(time(NULL));
	double aScore = rand() % 10001; // SBHack
	double aMinorScore = (rand() % 10001) / 100; // SBHack
	unsigned int aLevel = rand() % 10; // SBHack
	unsigned int aMode = rand() % 10; // SBHack
	scsetscore(app, aScore, &aMinorScore, &aLevel, &aMode); // SBHack
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

SC_Error_t scscoresfree(AppData_t *app) {
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
//									SC_ScoresController_Retain(app->scoresController);
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




/*
 * Game listing functions
 *
 *      SC_GAMES_SEARCH_LIST_ALL
 *      SC_GAMES_SEARCH_LIST_BUDDIES
 *      SC_GAMES_SEARCH_LIST_FEATURED
 *      SC_GAMES_SEARCH_LIST_POPULAR
 *      SC_GAMES_SEARCH_LIST_NEW
 *
 */

GameInfo_t *GetGameInfo(SC_Game_h oGame) {
	GameInfo_t *game = NULL;

	if(NULL != (game = (GameInfo_t *) calloc(1, sizeof(GameInfo_t)))) {
		game->name = strdup(SC_String_GetData(SC_Game_GetName(oGame)));
		game->imgurl = strdup(SC_String_GetData(SC_Game_GetImageUrl(oGame)));
		game->publisher = strdup(SC_String_GetData(SC_Game_GetPublisherName(oGame)));
		game->version = strdup(SC_String_GetData(SC_Game_GetVersion(oGame)));
		game->dlurl = strdup(SC_String_GetData(SC_Game_GetDownloadUrl(oGame)));
		game->desc = strdup(SC_String_GetData(SC_Game_GetDescription(oGame)));
		game->modes = SC_Game_GetModeCount(oGame);
	}
	return game;
}

SC_Bool_t scgameshasnextrange(AppData_t *app) {
	SC_Bool_t  rc;

	if(app->gamesController != NULL) {
		rc = SC_GamesController_HasNextRange(app->gamesController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scgamesgetnextrange(AppData_t *app) {
	SC_Error_t  rc;

	if(app->gamesController != NULL) {
		rc = SC_GamesController_LoadNextRange(app->gamesController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Bool_t scgameshasprevrange(AppData_t *app) {
	SC_Bool_t  rc;

	if(app->gamesController != NULL) {
		rc = SC_GamesController_HasPreviousRange(app->gamesController);
	} else {
		rc = false;
	}

	return rc;
}

SC_Error_t scgamesgetprevrange(AppData_t *app) {
	SC_Error_t  rc;

	if(app->gamesController != NULL) {
		rc = SC_GamesController_LoadPreviousRange(app->gamesController);
	} else {
		rc = false;
	}

	return rc;
}

void scgamesfree(AppData_t *app) {
	if(app->games != NULL) {
		for(unsigned int i=0; i< app->games_c; i++) {
			free((void *) app->games[i]->name);
			free((void *) app->games[i]->imgurl);
			free((void *) app->games[i]->publisher);
			free((void *) app->games[i]->version);
			free((void *) app->games[i]->dlurl);
			free((void *) app->games[i]->desc);
			free((void *) app->games[i]->modes);
		}

		free(app->games);
		app->games_c = 0;
		app->games = NULL;
	}


}

void gamesControllerCallback(void *userData, SC_Error_t completionStatus)
{
	SC_GameList_h glist;
	SC_Game_h oGame;
	unsigned int gcount;
	GameInfo_t **gamelist;

    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_GamesController_Release(app->gamesController); /* Cleanup Controller */
		app->gamesController = NULL;
		return;
	}

	if(app->games != NULL) {
		scgamesfree(app);
	}

    glist = SC_GamesController_GetGames(app->gamesController);
    gcount = SC_GameList_GetCount(glist);

    gamelist = (GameInfo_t **) calloc(sizeof(GameInfo_t *), gcount);
 	for(unsigned int i=0; i< gcount; i++) {
		oGame = SC_GameList_GetAt(glist, i);
		gamelist[i] = GetGameInfo(oGame);
		fprintf(stdout, "[Scoreloop Test] Game: %s v %s by %s - %s\n", gamelist[i]->name, gamelist[i]->version, gamelist[i]->publisher, gamelist[i]->dlurl); fflush(stdout);
	}

	app->games_c = gcount;
	app->games = gamelist;

}

SC_Error_t scgetgames(AppData_t *app, SC_GamesSearchList_t filter, unsigned int rangeLength = 0) {
	SC_Error_t rc;
	SC_Range_t range;

	range.offset = 0;
	range.length = rangeLength;

	if(app->client != NULL) {
		if((rc = SC_Client_CreateGamesController(app->client, &app->gamesController, gamesControllerCallback, app)) == SC_OK) {
			if((rc = SC_GamesController_LoadGames(app->gamesController, filter, range)) == SC_OK) {	// Triggers gamesControllerCallback
			}
		}
	} else {
		rc = SC_INVALID_STATE;
	}

	return rc;
}




/*
 * Money functions
 *
 *
 */

SC_Error_t sccreatemoney(AppData_t *app, unsigned int amount) {
	SC_Error_t rc;

	app->money = NULL;

	if(app->client != NULL) {
//		SC_Session_h session = SC_Client_GetSession(app->client);
//		SC_Money_h money_h = SC_Session_GetBalance(session);
		rc = SC_Client_CreateMoney(app->client, &app->money, amount);
	} else {
		rc = SC_INVALID_STATE;
	}

	return rc;
}




/*
 * Challenge functions
 *
 *
 */

void challengeControllerCallback(void *userData, SC_Error_t completionStatus)
{
    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_ChallengeController_Release(app->challengeController); /* Cleanup Controller */
		app->challengeController = NULL;
		return;
	}


}

SC_Error_t sccreatechallenge(AppData_t *app, unsigned int amount, unsigned int mode, unsigned int level, SC_User_h against = NULL) {
	SC_Error_t rc;

	if(app->client != NULL) {
		if(sccreatemoney(app, amount) == SC_OK) {
			if((rc = SC_Client_CreateChallengeController(app->client, &app->challengeController, challengeControllerCallback, app)) == SC_OK) {
				if((rc = SC_Client_CreateChallenge(app->client, app->money, against, mode, level, &app->challenge)) == SC_OK) {
					if(SC_ChallengeController_SetChallenge(app->challengeController, app->challenge) == SC_OK) {
						scgetchallengelist(app); // SBHack
					}
				}
			}
		}
	} else {
		rc = SC_INVALID_STATE;
	}

	return rc;
}

void challengesControllerCallback(void *userData, SC_Error_t completionStatus)
{
	SC_ChallengeList_h clist;
	SC_Challenge_h oChallenge;
	unsigned int ccount;
	ChallengeInfo_t **challengelist;

    /* Get the application from userData argument */
    AppData_t *app = (AppData_t *) userData;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_ChallengesController_Release(app->challengesController); /* Cleanup Controller */
		app->challengesController = NULL;
		return;
	}

/*
	if(app->challenges != NULL) {
		scchallengesfree(app);
	}
 */
    clist = SC_ChallengesController_GetChallenges(app->challengesController);
    ccount = SC_ChallengeList_GetCount(clist);

    challengelist = (ChallengeInfo_t **) calloc(sizeof(ChallengeInfo_t *), ccount);
 	for(unsigned int i=0; i< ccount; i++) {
		oChallenge = SC_ChallengeList_GetAt(clist, i);
//		challengelist[i] = GetChallengesInfo(oChallenge);
//		fprintf(stdout, "[Scoreloop Test] Game: %s v %s by %s - %s\n", gamelist[i]->name, gamelist[i]->version, gamelist[i]->publisher, gamelist[i]->dlurl); fflush(stdout);
	}

	app->challenges_c = ccount;
	app->challenges = challengelist;

}

SC_Error_t scgetchallengelist(AppData_t *app) {
	SC_Error_t rc;

	if(app->client != NULL) {
		if((rc = SC_Client_CreateChallengesController(app->client, &app->challengesController, challengesControllerCallback, app)) == SC_OK) {
			rc = SC_ChallengesController_LoadOpenChallenges(app->challengesController);
		}
	}

	return rc;
}



/*
 * Thread functions
 *
 * Requires call to scfreescores to start an new scgetscores
 *
 */

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
	    pthread_mutex_lock(&mutex);
	    threadHalt = false;
	    pthread_cond_signal(&cond);
	    pthread_mutex_unlock(&mutex);

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
	// Request thread to set prevent sleep to false and terminate
	pthread_mutex_lock(&mutex);
	threadHalt = true;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

    // Wait for the thread to terminate.
    void *exit_status;
    pthread_join(m_thread, &exit_status) ;

	// Clean conditional variable and mutex
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);

	m_thread = 0;
	threadHalt = true;

	// Tidy Up
	if(app.games != NULL) {
		scgamesfree(&app);
	}

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
	bool isThreadHalt;
	pthread_mutex_lock(&mutex);
	isThreadHalt = threadHalt;
	pthread_mutex_unlock(&mutex);
	return isThreadHalt;
}

