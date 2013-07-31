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

#include <scoreloop/scoreloopcore.h>
#include <pthread.h>

namespace webworks {

SC_Error_t errCode;
SC_Client_h client;
SC_InitData_t initData;

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

std::string TemplateNDK::start() {
	Json::FastWriter writer;
	Json::Reader reader;
	Json::Value root;
	Json::Value res;

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

		errCode = SC_Client_New(&client, &initData, aGameId, aGameSecret, aGameVersion, aCurrency, aLanguageCode);

		if(errCode == SC_OK) {
			templateStartThread();
		}
	}

	res["errCode"] = errCode;
	return writer.write(res);
}


void TemplateNDK::stop() {
	if(!threadHalt) {
		templateStopThread();
	}
}

// Thread functions
// The following functions are for controlling the main Scoreloop Thread

// The actual thread (must appear before the startThread method)
// Loops and runs the Scoreloop event loop

void* TemplateThread(void* unused) {
	TemplateNDK *pParent = static_cast<TemplateNDK *>(parent);

	// Loop calls the callback function and continues until stop is set
	while (!pParent->isThreadHalt()) {
		// Loop calls Scoreloop and continues until stop is set
		SC_HandleCustomEvent(&initData, SC_FALSE);

		sleep(1);
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
	SC_Client_Release(client);

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
