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

// Remove %22 from the start and end of JSON string

char* strpc22del(const char *str) {
	char *ns;
	size_t l;

	ns = strdup(str);
	l = strlen(ns);

	if(strncmp(ns, "%22", 3) == 0) {
		memmove(ns, ns + 3, l - 2);
		l -= 3;
	}

	if(strncmp(ns + l - 3, "%22", 3) == 0) {
		memset(ns + l - 3, 0, 3);
	}

	return ns;
}

namespace webworks {

AppData_t app;
SC_InitData_t initData;

SC_Error_t scgetuser(AppData_t *app);

TemplateNDK::TemplateNDK(TemplateJS *parent) {
	m_pParent = parent;
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


TemplateNDK::~TemplateNDK() {
	if(!threadHalt) {
		templateStopThread();
	}
}

// These methods are the true native code we intend to reach from WebWorks

std::string TemplateNDK::start(const std::string& arg) {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;

	char *aGameId = NULL;
	char *aGameSecret = NULL;
	char *aGameVersion = NULL;
	char *aCurrency = NULL;
	char *aLanguageCode = NULL;
	int rc = 0;


	bool parse = reader.parse(arg, root);

 	if (parse) {
 		aGameId = strpc22del((const char *) root["slGameId"].asCString());
 		aGameSecret = strpc22del((const char *) root["slGameSecret"].asCString());
 		aGameVersion = strpc22del((const char *) root["slGameVersion"].asCString());
 		aCurrency = strpc22del((const char *) root["slCurrency"].asCString());
 		aLanguageCode = strpc22del((const char *) root["slLanguageCode"].asCString());
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
	res["errCode"] = rc;
	res["aGameId"] = aGameId;
	res["aGameSecret"] = aGameSecret;
	res["aGameVersion"] = aGameVersion;
	res["aCurrency"] = aCurrency;
	res["aLanguageCode"] = aLanguageCode;

	free(aGameId);
	free(aGameSecret);
	free(aGameVersion);
	free(aCurrency);
	free(aLanguageCode);

 	return writer.write(res);
}




/*
 * Exposed functions
 *
 */

void TemplateNDK::getUser() {
//	scgetuser(&app);
	getUserCallback(&app);
}

// Asynchronous callback with JSON data object
void TemplateNDK::getUserCallback(AppData_t *app) {
	Json::FastWriter writer;
	Json::Value root;
	root["templateJSONString"] = "JSON String";
	root["templateJSONInt"] = 85;

	std::string event = "community.scoreloop.getUserCallback";
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
	SC_Error_t rc = SC_INVALID_STATE;

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

	/* We don't need the UserController anymore, so release it */
	SC_UserController_Release(app->userController);
	app->userController = NULL;

    TemplateNDK* ndk = static_cast<TemplateNDK*>(app->m_pTemplateNDK);
	ndk->getUserCallback(app);
}


SC_Error_t scgetuser(AppData_t *app) {
	SC_Error_t rc = SC_INVALID_STATE;

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

	return rc;
}

/*
 * Scores functions
 *
 */





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
