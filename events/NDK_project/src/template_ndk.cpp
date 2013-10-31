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

#include <bps/bps.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/led.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include <string>
#include <sstream>
#include <json/reader.h>
#include <json/writer.h>
#include <pthread.h>
#include "template_ndk.hpp"
#include "template_js.hpp"

#define MUTEX_LOCK() pthread_mutex_trylock(&m_lock)
#define MUTEX_UNLOCK() pthread_mutex_unlock(&m_lock)

int ecnt = 0;
int qcnt = 0;

namespace webworks {

int TemplateNDK::m_eventChannel;
int TemplateNDK::m_coid;
bool TemplateNDK::m_eventsEnabled;
pthread_mutex_t TemplateNDK::m_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t TemplateNDK::m_thread = 0;

TemplateNDK::TemplateNDK(TemplateJS *parent) {
	m_pParent = parent;
    bps_initialize();
}

TemplateNDK::~TemplateNDK() {
    bps_shutdown();
}

// BPS Event handler functions

void *TemplateNDK::HandleEvents(void *args)
{
	TemplateJS *parent = reinterpret_cast<TemplateJS *>(args);

    // create channel for events
    m_eventChannel = ChannelCreate(0);
    m_coid = ConnectAttach(ND_LOCAL_NODE, 0, m_eventChannel, _NTO_SIDE_CHANNEL, 0);

    m_eventsEnabled = true;

    navigator_request_events(0);

    bps_event_t *event = NULL;

    for (;;) {
        MUTEX_LOCK();
        bps_get_event(&event, -1);
        MUTEX_UNLOCK();
        if (event) {
            if (bps_event_get_domain(event) == navigator_get_domain()) {
            	unsigned int evt = bps_event_get_code(event);

            	led_request_rgb("community.events",0x00FFFFFF,1);

                Json::FastWriter writer;
                Json::Value root;

                root["data"] = evt;

                ecnt++;

                parent->NotifyEvent("community.events " + writer.write(root));
            }
        }
    }

    //clean up channels
    ConnectDetach(m_coid);
    ChannelDestroy(m_eventChannel);

    return NULL;
}

void TemplateNDK::StartEvents()
{
	if(!m_eventsEnabled) {
		if (!m_thread) {
			int error = pthread_create(&m_thread, NULL, HandleEvents, static_cast<void *>(m_pParent));

			if (error) {
				m_thread = 0;
			} else {
				MUTEX_LOCK();
			}
		}
	}

}

void TemplateNDK::StopEvents()
{
	if(m_eventsEnabled) {
		if (m_thread) {
			pthread_join(m_thread, NULL);
			m_thread = 0;
			m_eventsEnabled = false;
		}
	}
}

// These methods are the true native code we intend to reach from WebWorks

std::string TemplateNDK::getcount()
{
    Json::FastWriter writer;
    Json::Value root;

    qcnt++;

    root["ecnt"] = ecnt;
    root["qcnt"] = qcnt;

    return writer.write(root);

}


} /* namespace webworks */
