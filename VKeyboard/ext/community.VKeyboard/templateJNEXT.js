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

///////////////////////////////////////////////////////////////////
// JavaScript wrapper for JNEXT plugin for connection
///////////////////////////////////////////////////////////////////

var template,
	_event = require("../../lib/event");

JNEXT.Template = function () {
	var self = this,
		hasInstance = false;

	self.getId = function () {
		return self.m_id;
	};

	self.init = function () {
		if (!JNEXT.require("libVKeyboard")) {
			return false;
		}

		self.m_id = JNEXT.createObject("libVKeyboard.TemplateJS");

		if (self.m_id === "") {
			return false;
		}

		JNEXT.registerEvents(self);
	};

	// ************************
	// Enter your methods here
	// ************************

	// calls into InvokeMethod(string command) in template_js.cpp
	self.VKshow = function () {
		return JNEXT.invoke(self.m_id, "VKshow");
	};
	self.VKhide = function () {
		return JNEXT.invoke(self.m_id, "VKhide");
	};
	self.VKsetLayout = function (args) {
		return JNEXT.invoke(self.m_id, "VKsetLayout " + JSON.stringify(args));
	};
	self.VKgetHeight = function () {
		return Number(JNEXT.invoke(self.m_id, "VKgetHeight"));
	};
	self.VKhasPhysicalKeyboard = function () {
		return Number(JNEXT.invoke(self.m_id, "VKhasPhysicalKeyboard"));
	};
	
	self.onEvent = function (strData) {
		var arData = strData.split(" "),
			strEventDesc = arData[0],
			jsonData;
		// Event names are set in native code when fired,
		// and must be checked here.
		if (strEventDesc === "community.VKeyboard.VKvisible") {
			_event.trigger("community.VKeyboard.VKvisible");
		} else if (strEventDesc === "community.VKeyboard.VKhidden") {
			_event.trigger("community.VKeyboard.VKhidden");
		} else if (strEventDesc === "community.VKeyboard.VKchangeHeight") {
			_event.trigger("community.VKeyboard.VKchangeHeight");
		}
	};

	// ************************
	// End of methods to edit
	// ************************
	self.m_id = "";

	self.getInstance = function () {
		if (!hasInstance) {
			hasInstance = true;
			self.init();
		}
		return self;
	};

};

template = new JNEXT.Template();

module.exports = {
	template: template
};
