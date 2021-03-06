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

	_self.start = function (callback) {
		window.webworks.event.add(_ID, "community.events.eventCallback", callback);
		window.webworks.execSync(_ID, "start", null);
	};
	
	_self.stop = function () {
		window.webworks.execSync(_ID, "stop", null);
	};
	
	_self.getcount = function () {
		return window.webworks.execSync(_ID, "getcount", null);
	};
	
module.exports = _self;
