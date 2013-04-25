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
#include <json/reader.h>
#include <json/writer.h>
#include <pthread.h>
#include "template_ndk.hpp"
#include "template_js.hpp"

#include <bb/device/HardwareInfo>
#include <bps/virtualkeyboard.h>

namespace webworks {

int vkestatus;

TemplateNDK::TemplateNDK(TemplateJS *parent) {
	m_pParent = parent;
	m_thread = 0;
	pthread_cond_t cond  = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	threadHalt = true;

	vkHeight = -1;
	vkVisible = false;
	bps_initialize();
	vkestatus = virtualkeyboard_request_events(0);
	templateStartThread();
}

TemplateNDK::~TemplateNDK() {
	templateStopThread();
	virtualkeyboard_stop_events(0);
	bps_shutdown();
}


// These methods are the true native code we intend to reach from WebWorks
void TemplateNDK::VKshow() {
	virtualkeyboard_show();
}

void TemplateNDK::VKhide() {
	virtualkeyboard_hide();
}

void TemplateNDK::VKsetLayout(const std::string& arg) {
 	int layout;
	int enter;
	Json::Reader reader;
	Json::Value root;

	bool parse = reader.parse(arg, root);

 	if (parse) {
		layout = (int) strtol(root["layout"].asCString(), NULL, 10);;
		enter = (int) strtol(root["enter"].asCString(), NULL, 10);;
		virtualkeyboard_change_options((virtualkeyboard_layout_t) layout, (virtualkeyboard_enter_t) enter);
	}

}

std::string TemplateNDK::VKgetHeight() {
	std::stringstream ss;

	if(vkHeight == -1) { // vkHeight gets updated by the event loop
		if(virtualkeyboard_get_height(&vkHeight) != BPS_SUCCESS) {
			vkHeight = -1;
		}
	}

	ss << vkHeight;

	return ss.str();
}

std::string TemplateNDK::VKhasPhysicalKeyboard() {
	bb::device::HardwareInfo hwInfo;
	return hwInfo.isPhysicalKeyboardDevice() ? "1" : "0";
}

// Event Loop

void TemplateNDK::getEvents() {
    bps_event_t *event = NULL;

    if(bps_get_event(&event, 0) == BPS_SUCCESS) {
    	if (event) {
    		int event_domain = bps_event_get_domain(event);

    		if (event_domain == virtualkeyboard_get_domain()) {
    			uint16_t code = bps_event_get_code(event);

    			switch (code)
    			{
    				case VIRTUALKEYBOARD_EVENT_VISIBLE:
    				{
    					vkVisible = true;
    					m_pParent->NotifyEvent("community.VKeyboard.VKvisible");
    					break;
    				}
    				case VIRTUALKEYBOARD_EVENT_HIDDEN:
    				{
    					vkVisible = false;
    					m_pParent->NotifyEvent("community.VKeyboard.VKhidden");
    					break;
    				}
    				case VIRTUALKEYBOARD_EVENT_INFO:
    				{
    					vkHeight = virtualkeyboard_event_get_height(event);
    					m_pParent->NotifyEvent("community.VKeyboard.VKchangeHeight");
    					break;
    				}
    			}
    		}
        }
    }
}


// Thread functions
// The following functions are for controlling a Thread in the extension

// The actual thread (must appear before the startThread method)
// Loops and runs the callback method
void* TemplateThread(void* parent) {
	TemplateNDK *pParent = static_cast<TemplateNDK *>(parent);

	// Loop calls the callback function and continues until stop is set
	while (!pParent->isThreadHalt()) {
		if(vkestatus == BPS_SUCCESS) {
			pParent->getEvents();
		}
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
