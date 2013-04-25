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
	std::string templateStartThread();
	std::string templateStopThread();
	bool isThreadHalt();

	// The extension methods are defined here
	void VKshow();
	void VKhide();
	void VKsetLayout(const std::string& arg);
	std::string VKgetHeight();
	std::string VKhasPhysicalKeyboard();
    void getEvents();

	int vkHeight;

//	void templateThreadCallback();

private:
	TemplateJS *m_pParent;
	bool vkVisible;
	bool threadHalt;
	pthread_t m_thread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

} // namespace webworks

#endif /* TEMPLATENDK_H_ */
