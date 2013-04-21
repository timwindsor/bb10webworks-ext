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

var template = require("./templateJNEXT").template,
	_event = require("../../lib/event"),
    _utils = require("../../lib/utils");

module.exports = {

	// Code can be declared and used outside the module.exports object,
	// but any functions to be called by client.js need to be declared
	// here in this object.

	// These methods call into templateJNEXT.js which handles the
	// communication through the JNEXT plugin to template_js.cpp
	VKshow: function (success, fail) {
		success(template.getInstance().VKshow());
	},
	VKhide: function (success, fail) {
		success(template.getInstance().VKhide());
	},
	VKsetLayout: function (success, fail, args) {
		var layout = JSON.parse(decodeURIComponent(args["layout"]));
		var enter = JSON.parse(decodeURIComponent(args["enter"]));
		var args = { layout: String(layout), enter: String(enter) };
		success(template.getInstance().VKsetLayout(args));
	},
	VKgetHeight: function (success, fail) {
		success(template.getInstance().VKgetHeight());
	},
	VKhasPhysicalKeyboard: function (success, fail) {
		success(template.getInstance().VKhasPhysicalKeyboard());
	}
};