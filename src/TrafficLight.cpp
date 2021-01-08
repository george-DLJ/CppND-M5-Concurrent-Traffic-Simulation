#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

/**
 * NOTE for mentor: 
 * The cars are sometimes ignoring the traffic ligths whether:
 * - stay stopeed on a green light or 
 * - they enter some intersections when traffic light is red.
 * I added two _queue.clear one on receive() and one on send() following
 * Knowledge Base recommentations: 
 *  - https://knowledge.udacity.com/questions/116321
 *  - https://knowledge.udacity.com/questions/267031
 * Since  then the issues described above have notably reduced but 
 * the stuck cars on green  still happens. Specially visible when increasing 
 * lights Phases to larger periods of time, e.g 5-20 seconds.
 * 
 * QUESTIONS:
 * - Why is above described problem happening? 
 * - Should not msg queues precisely avoid that?
 * - If a car goes through a green light and there is a second car on the queue, 
 *    how does this second car knows the state of the Traffic light if the 
 *    traffic light state messsage has been removed from the queue?
 * 
 * CODE REVIEW: 
 *  - _queue.pop_back(); has been removed, because we are clearing the queue afterwards and 
 *             makes no sense pop_back if we are going to clear anyway.
 */ 
template <typename T>          
T MessageQueue<T>::receive() 
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    std::unique_lock<std::mutex> uLock(_mutex);
    _condition.wait(uLock, [this] {return !_queue.empty();});

    T msg= std::move(_queue.back()); 
    _queue.clear(); //see: https://knowledge.udacity.com/questions/267031 ; 
    return msg;
}

/**
 * CODE REVIEW: _queue must not be cleared before pushing an message into it.
 */
template <typename T>            
void MessageQueue<T>::send(T &&msg) 
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    
    //perform queue modification under the Lock (until end of scope):
    std::lock_guard<std::mutex> uLock(_mutex); 
    _queue.push_back(std::move(msg)); 
    _condition.notify_one(); 
}


/* Implementation of class "TrafficLight" */

 
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true)
    {
        if(_lightPhaseMsgs.receive() == TrafficLightPhase::green)
            return; 
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started 
    //         in a thread when the public method „simulate“ is called. To do 
    //         this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

/**
 * NOTE for mentor: 
 *  Please check if this method is correct, as the strange
 *  behavior of the cars could be related to a conceptual error here.            
 * 
 * NOTE:  virtual function which is executed in a thread
 */
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 
    
    //NOTE: Implementation using same technique used on Vehicle::drive()
    // 0. Set Phase Duration to a (pseudo)random value between 4-6 secs
    std::random_device rd;
    std::mt19937 eng(rd()); //mersenne twister engin pseudo-random nr generator
    std::uniform_int_distribution<> distr(4000, 6000); // Traffic light phase duration in milliseconds;
    long phaseDuration = (long)distr(eng); // Convert Traffic light phase duration to milliseconds;

    // 1. Init stop watch
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    lastUpdate = std::chrono::system_clock::now();
    while (true){
        // 2. Compute time difference to stop watch
        long elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (elapsedTime >= phaseDuration) {
            // 3. Toggle between red and green
            _currentPhase = _currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red;
            // 4. Send update message to queue
            _lightPhaseMsgs.send(std::move(_currentPhase));
            // 5. reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();
        }
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

