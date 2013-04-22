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
#include <bb/device/HardwareInfo>
#include <bps/virtualkeyboard.h>
#include <pthread.h>
#include "template_ndk.hpp"
#include "template_js.hpp"

namespace webworks {

TemplateNDK::TemplateNDK(TemplateJS *parent) {
	m_pParent = parent;
	vkHeight = -1;
	vkVisible = false;
	bps_initialize();
//	m_pParent->StartEvents();
}

TemplateNDK::~TemplateNDK() {
//	m_pParent->StopEvents();
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

	if(vkHeight == -1) {
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

int TemplateNDK::WaitForEvents() {
    int status = virtualkeyboard_request_events(0);

    bps_event_t *event = NULL;

    for (;;) {
        bps_get_event(&event, -1);
        if (event) {
            int event_domain = bps_event_get_domain(event);

            if (event_domain == virtualkeyboard_get_domain()) {
            	uint16_t code = bps_event_get_code(event);

                switch (code)
                {
                	case VIRTUALKEYBOARD_EVENT_VISIBLE:
                	{
                		vkVisible = true;
                		m_pParent->NotifyEvent("VKvisible");
                		break;
                	}
                	case VIRTUALKEYBOARD_EVENT_HIDDEN:
                	{
                		vkVisible = false;
                		m_pParent->NotifyEvent("VKhidden");
                		break;
                	}
                	case VIRTUALKEYBOARD_EVENT_INFO:
                	{
                		vkHeight = virtualkeyboard_event_get_height(event);
                		m_pParent->NotifyEvent("VKchangeHeight");
                		break;
                	}
                }
            }

        }
    }

    return (status == BPS_SUCCESS) ? 0 : 1;
}

static void SendEndEvent(){
	virtualkeyboard_request_events(0);
}




} /* namespace webworks */
