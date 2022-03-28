# cli training diary

Simple C++ Sqlite wrapper to interact with your own local training database. It's capable of adding new training sessions to the database as well as printing the details about past training sessions.

Know how:

* While requesting training data of the specific day, if you type `today` it will print the today's training
* While requesting training data between the period of time, if you type `week` it will print the training sessions done through past week
* if you type `week -s` it will display only the names of exercises done through past week

 To compile simply run: `g++ training.cpp -l sqlite3`
