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
		if (!JNEXT.require("libscoreloop")) {
			return false;
		}

		self.m_id = JNEXT.createObject("libscoreloop.TemplateJS");

		if (self.m_id === "") {
			return false;
		}

		JNEXT.registerEvents(self);
	};

	// ************************
	// Enter your methods here
	// ************************

	// calls into InvokeMethod(string command) in template_js.cpp
	self.getbuddylist = function () {
		return JNEXT.invoke(self.m_id, "getbuddylist");
	};
	self.getchallengelist = function () {
		return JNEXT.invoke(self.m_id, "getchallengelist");
	};
	self.getuser = function () {
		return JNEXT.invoke(self.m_id, "getuser");
	};
	self.readlog = function () {
		return JNEXT.invoke(self.m_id, "readlog");
	};
	self.setscore = function (input) {
		return JNEXT.invoke(self.m_id, "setscore " + JSON.stringify(input));
	};
	self.setchallenge = function (input) {
		return JNEXT.invoke(self.m_id, "setchallenge " + JSON.stringify(input));
	};
	self.getleaders = function (input) {
		return JNEXT.invoke(self.m_id, "getleaders " + JSON.stringify(input));
	};
	self.start = function (input) {
		return JNEXT.invoke(self.m_id, "start " + JSON.stringify(input));
	};
	
	// Fired by the Event framework (used by asynchronous callbacks)
	self.onEvent = function (strData) {
		var arData = strData.split(" "),
		strEventDesc = arData[0],
		jsonData;
		
		// Event names are set in native code when fired,
		// and must be checked here.
		if (strEventDesc === "community.scoreloop.getUserCallback") {
			// Slice off the event name and the rest of the data is our JSON
			jsonData = arData.slice(1, arData.length).join(" ");
			_event.trigger("community.scoreloop.getUserCallback", JSON.parse(jsonData));
		} else if (strEventDesc === "community.scoreloop.getBuddyListCallback") {
			// Slice off the event name and the rest of the data is our JSON
			jsonData = arData.slice(1, arData.length).join(" ");
			_event.trigger("community.scoreloop.getBuddyListCallback", JSON.parse(jsonData));
		} else if (strEventDesc === "community.scoreloop.getLeadersCallback") {
			// Slice off the event name and the rest of the data is our JSON
			jsonData = arData.slice(1, arData.length).join(" ");
			_event.trigger("community.scoreloop.getLeadersCallback", JSON.parse(jsonData));
		} else if (strEventDesc === "community.scoreloop.setChallengeCallback") {
			// Slice off the event name and the rest of the data is our JSON
			jsonData = arData.slice(1, arData.length).join(" ");
			_event.trigger("community.scoreloop.setChallengeCallback", JSON.parse(jsonData));
		} else if (strEventDesc === "community.scoreloop.getChallengeListCallback") {
			// Slice off the event name and the rest of the data is our JSON
			jsonData = arData.slice(1, arData.length).join(" ");
			_event.trigger("community.scoreloop.getChallengeListCallback", JSON.parse(jsonData));
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
