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
#define LOG(fmt, args...)   do { fprintf(stdout, "[Scoreloop Sample] " fmt "\n", ##args); fflush(stdout); } while (0);

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

void userget() {
	scgetuser(&app);
}


// These methods are the true native code we intend to reach from WebWorks

std::string start() {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;

    memset(&app, 0, sizeof(AppData_t));

	char *aGameId = (char *) "cdc1ee7e-403b-4d68-ac1a-cb1ebf36f782";
	char *aGameSecret = (char *) "dNVkPkrrp68BOeOu4NqUx1ZeF+iWY21BEFF4hT4A7spAuIYUhiE49w==";
	char *aGameVersion = (char *) "1.0";
	char *aCurrency = (char *) "ACF";
	char *aLanguageCode = (char *) "en";
	int errCode = 0;

	// aGameId, aGameSecret and aCurrency are const char strings that you obtain from Scoreloop.
	// aGameVersion should be your current game version.
	// aLanguageCode specifies the language support for localization in awards,
	// for example, "en" for English, which is the default language.

	if(threadHalt) {
		initData.runLoopType = SC_RUN_LOOP_TYPE_CUSTOM;

		errCode = SC_Client_New(&app.client, &initData, aGameId, aGameSecret, aGameVersion, aCurrency, aLanguageCode);

		if(errCode == SC_OK) {
			templateStartThread();
		}
	}


	res["errCode"] = errCode;
	return writer.write(res);
}


void stop() {
	if(!threadHalt) {
		templateStopThread();
	}
}

UserInfo_t *GetUserInfo(SC_User_h user, bool isBuddy) {
	UserInfo_t *uinfo;
	if(NULL != (uinfo = (UserInfo_t *) calloc(1, sizeof(UserInfo_t)))) {
		/* Get user data */
		uinfo->user = user;
		uinfo->login = SC_String_GetData(SC_User_GetLogin(user));
		if(isBuddy) { // Buddy doesn't have access to email
			uinfo->email = NULL;
		} else {
			uinfo->email = SC_String_GetData(SC_User_GetEmail(user));
		}
		uinfo->imgurl = SC_String_GetData(SC_User_GetImageUrl(user));
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
    UserInfo_t *uinfo;
    void **buddylist = NULL;

	/* Check completion status */
	if (completionStatus != SC_OK) {
		SC_UsersController_Release(app->usersController); /* Cleanup Controller */
		return;
	}

	SC_UserList_h buddies = SC_UsersController_GetUsers(app->usersController);

	if(buddies) {
		unsigned int ulist = SC_UserList_GetCount(buddies);
		UserInfo_t **buddylist = (UserInfo_t **) malloc(sizeof(UserInfo_t) * ulist);
		for(unsigned int i=0; i< ulist; i++) {
			buddy = SC_UserList_GetAt(buddies, i);
			buddylist[i] = GetUserInfo(buddy, true);
	        LOG("User: %s", buddylist[i]->login);
		}

		app->buddy_c = ulist;
		app->buddies = buddylist;
	}
    SC_UsersController_Release(app->usersController);
}

void scgetbuddies(AppData_t *app) {
	//create user controller
	SC_Error_t rc = SC_Client_CreateUsersController(app->client, &app->usersController, usersControllerCallback, app);

    /* Make the asynchronous request */
    rc = SC_UsersController_LoadBuddies(app->usersController, app->UserInfo->user);
    if (rc != SC_OK) {
        SC_UsersController_Release(app->usersController);
        return;
    }

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

	scgetbuddies(app);

	/* We don't need the UserController anymore, so release it */
	SC_UserController_Release(app->userController);
}


void scgetuser(AppData_t *app) {
	//create user controller
	SC_Error_t rc = SC_Client_CreateUserController(app->client, &app->userController, userControllerCallback, app);

	if(rc == SC_OK) {
		/* Make the asynchronous request */
		rc = SC_UserController_LoadUser(app->userController);
		if (rc != SC_OK) {
			SC_UserController_Release(app->userController);
			return;
		}
	}
}

// Thread functions
// The following functions are for controlling the main Scoreloop Thread

// The actual thread (must appear before the startThread method)
// Loops and runs the Scoreloop event loop

void* TemplateThread(void* unused) {

	// Loop calls Scoreloop and continues until stop is set
	while (!isThreadHalt()) {
		SC_HandleCustomEvent(&initData, SC_FALSE);

		sleep(1);
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
	SC_User_Release(app.UserInfo->user);
	free(app.UserInfo);
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

