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
#include <json/writer.h>
#include <pthread.h>
#include "template_ndk.hpp"
#include "template_js.hpp"

#include <bb/device/DisplayInfo>

namespace webworks {

TemplateNDK::TemplateNDK(TemplateJS *parent) {
}

TemplateNDK::~TemplateNDK() {
}


// These methods are the true native code we intend to reach from WebWorks

std::string TemplateNDK::SDgetSize() {
	bb::device::DisplayInfo display;
	Json::FastWriter writer;
	Json::Value root;

	root["physicalWidth"] = display.physicalSize().width();
	root["physicalHeight"] = display.physicalSize().height();
	root["pixelWidth"] = display.pixelSize().width();
	root["pixelHeight"] = display.pixelSize().height();

 	return writer.write(root);
}


} /* namespace webworks */
