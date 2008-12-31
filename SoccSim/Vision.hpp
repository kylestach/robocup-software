#ifndef VISION_HPP_
#define VISION_HPP_

#include <QThread>

#include "Physics/Env.hpp"

class Vision : public QThread
{
	public:
		Vision(Env* env);
		~Vision();

                uint64_t timestamp();

	protected:
		void run();

	private:

	    Env* _env;

	    unsigned int _id, _fps;
};

#endif /* VISION_HPP_ */
