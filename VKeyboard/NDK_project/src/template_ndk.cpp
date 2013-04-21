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
	vkbHeight = -1;
	bps_initialize();
}

TemplateNDK::~TemplateNDK() {
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

	if(vkbHeight == -1) {
		if(virtualkeyboard_get_height(&vkbHeight) != BPS_SUCCESS) {
			vkbHeight = -1;
		}
	}

	ss << vkbHeight;

	return ss.str();
}

std::string TemplateNDK::VKhasPhysicalKeyboard() {
	bb::device::HardwareInfo hwInfo;
	return hwInfo.isPhysicalKeyboardDevice() ? "1" : "0";
}



} /* namespace webworks */
