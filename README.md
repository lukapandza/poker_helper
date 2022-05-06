# poker_helper

poker_helper is a C++ project which is designed to run in terminal alongside a live / online poker game. The user manually inputs some information as the game progresses and the program runs simulations given the current data to provide a probability of the user's hand being a winning hand. The program also stores info about the outcomes hand to hand and writes these out to a file at the end of a session for some data collection.

The project is built around a [very old approach to the problme of ranking hands in a Texas Hold 'Em poker game](https://web.archive.org/web/20111103160502/http://www.codingthewheel.com/archives/poker-hand-evaluator-roundup#2p2). It is written in C++ primarily for lookup efficiency and fast hand simulation. 

An additional part of the program is a pre-computed table of winning probabilities in the pre-flop stage which I have generated myself. This significantly reduces the computational time required per simulation. 

The opponents are simulated as simply competent by checking their pre-flop win probability and sampling a tight normal distribution to determine weather they would fold or not. This step is repeated in the following rounds as well. This yielded better results than simply playing out every game and it errs on the side of caution when estimating the user's win probability.
