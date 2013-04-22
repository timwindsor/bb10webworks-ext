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
#include "../public/tokenizer.h"
#include "template_js.hpp"
#include "template_ndk.hpp"

using namespace std;

static bool eventsInitialized = false;

// Notifies JavaScript of an event
void TemplateJS::NotifyEvent(const std::string& event)
{
    std::string eventString = m_id;
    eventString.append(" ");
    eventString.append(event);
    SendPluginEvent(eventString.c_str(), m_pContext);
}

void TemplateJS::StartEvents()
{
    if (!m_thread) {
        eventsInitialized = false;
        int error = pthread_create(&m_thread, NULL, VKEventThread, static_cast<void *>(this));

        if (error) {
            m_thread = 0;
        }
    }
}

void TemplateJS::StopEvents()
{
    if (m_thread) {
        // Ensure that the secondary thread was initialized
        while (!eventsInitialized);

        webworks::TemplateNDK::SendEndEvent();
        eventsInitialized = false;
        pthread_join(m_thread, NULL);
        m_thread = 0;
    }
}

void* TemplateJS::VKEventThread(void *parent)
{
    // Parent object is casted so we can use it
	TemplateJS *pParent = static_cast<TemplateJS *>(parent);

    webworks::TemplateNDK *VKEvents = new webworks::TemplateNDK(pParent);

    eventsInitialized = true;
    VKEvents->WaitForEvents();

    return NULL;
}


/**
 * Default constructor.
 */
TemplateJS::TemplateJS(const std::string& id) :
		m_id(id) {
	m_pTemplateController = new webworks::TemplateNDK(this);
}

/**
 * TemplateJS destructor.
 */
TemplateJS::~TemplateJS() {
	if (m_pTemplateController)
		delete m_pTemplateController;
}

/**
 * This method returns the list of objects implemented by this native
 * extension.
 */
char* onGetObjList() {
	static char name[] = "TemplateJS";
	return name;
}

/**
 * This method is used by JNext to instantiate the TemplateJS object when
 * an object is created on the JavaScript server side.
 */
JSExt* onCreateObject(const string& className, const string& id) {
	if (className == "TemplateJS") {
		return new TemplateJS(id);
	}

	return NULL;
}

/**
 * Method used by JNext to determine if the object can be deleted.
 */
bool TemplateJS::CanDelete() {
	return true;
}

/**
 * It will be called from JNext JavaScript side with passed string.
 * This method implements the interface for the JavaScript to native binding
 * for invoking native code. This method is triggered when JNext.invoke is
 * called on the JavaScript side with this native objects id.
 */
string TemplateJS::InvokeMethod(const string& command) {
	// command appears with parameters following after a space
	int index = command.find_first_of(" ");
	std::string strCommand = command.substr(0, index);
	std::string arg = command.substr(index + 1, command.length());

	// based on the command given, run the appropriate method in template_ndk.cpp
	if (strCommand == "VKshow") {
		m_pTemplateController->VKshow();
	} else if (strCommand == "VKhide") {
		m_pTemplateController->VKhide();
	} else if (strCommand == "VKsetLayout") {
		m_pTemplateController->VKsetLayout(arg);
	} else if (strCommand == "VKgetHeight") {
		return m_pTemplateController->VKgetHeight();
	} else if (strCommand == "VKhasPhysicalKeyboard") {
		return m_pTemplateController->VKhasPhysicalKeyboard();
	}

	strCommand.append(";");
	strCommand.append(command);
	return strCommand;
}
