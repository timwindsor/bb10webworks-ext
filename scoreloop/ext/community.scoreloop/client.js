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

var _self = {},
	_ID = require("./manifest.json").namespace;

	// These methods are called by your App's JavaScript
	// They make WebWorks function calls to the methods
	// in the index.js of the Extension

	// Simple Synchronous test function to get a string
	_self.setscore = function (args) {
//		window.webworks.event.once(_ID, "community.scoreloop.setScoreCallback", callback);
		return window.webworks.execSync(_ID, "setscore", args);
	};
	_self.getbuddylist = function (callback) {
		window.webworks.event.once(_ID, "community.scoreloop.getBuddyListCallback", callback);
		return window.webworks.execAsync(_ID, "getbuddylist", null);
	};
	_self.getleaders = function (args, callback) {
		window.webworks.event.once(_ID, "community.scoreloop.getLeadersCallback", callback);
		return window.webworks.execAsync(_ID, "getleaders", args);
	};
	_self.getuser = function (callback) {
		window.webworks.event.once(_ID, "community.scoreloop.getUserCallback", callback);
		return window.webworks.execAsync(_ID, "getuser", null);
	};
	_self.readlog = function () {
		return JSON.parse(window.webworks.execSync(_ID, "readlog", null));
	};
	_self.start = function (args) {
		return window.webworks.execSync(_ID, "start", args);
	};

module.exports = _self;
