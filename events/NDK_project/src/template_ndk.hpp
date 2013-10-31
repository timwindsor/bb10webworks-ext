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

#ifndef TEMPLATENDK_HPP_
#define TEMPLATENDK_HPP_

#include <string>
#include <pthread.h>

class TemplateJS;

namespace webworks {

class TemplateNDK {
public:
	explicit TemplateNDK(TemplateJS *parent = NULL);
	virtual ~TemplateNDK();

    // Events related functions
    static void *HandleEvents(void *);
    void StartEvents();
    void StopEvents();
    std::string getcount();

	// The extension methods are defined here

private:
	TemplateJS *m_pParent;
    static int m_eventChannel;
    static int m_coid;
    static bool m_eventsEnabled;
    static pthread_mutex_t m_lock;
    static pthread_t m_thread;
};

} // namespace webworks

#endif /* TEMPLATENDK_H_ */
