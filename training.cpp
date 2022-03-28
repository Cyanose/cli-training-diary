#include "regex"
#include "iostream"
#include "sstream"
#include "string"
#include "sqlite3.h"
#include "ctime"
#include "cstdio"

using namespace std;

stringstream int_to_str; 
time_t now = time(0);
tm *ltm = localtime(&now);
sqlite3 *db;
int rc;
char *err_msg;

static int callback(void *not_used, int argc, char **argv, char **az_col_name){
	for(int i=0;i<argc;i++)
		printf("%s = %s\n", az_col_name[i], argv[i] ? argv[i]: "Null");
	printf("\n");
	return 0;
};

/* function to embed string with "" for sql queries */
void embed(string& str){
	str.insert(str.begin(),'"'); 
	str.insert(str.end(),'"'); 
};
/* functions to format date for sql queries */
string format_date(){
	string ret,day, month, year;
	int_to_str<<ltm->tm_mday;
	int_to_str>>day;
	ret+=day+".";
	int_to_str.clear();

	int_to_str <<1+ltm->tm_mon;
	int_to_str>>month;
	ret+=month+".";
	int_to_str.clear();

	int_to_str<<1900+ltm->tm_year;
	int_to_str>>year;
	ret+=year;
	int_to_str.clear();

	return ret;
};
string format_date(int day){
	string ret,day_string,month,year;
	int_to_str<<day;
	int_to_str>>day_string;
	ret+=day_string+".";
	int_to_str.clear();
	int_to_str <<1+ltm->tm_mon;
	int_to_str>>month;
	ret+=month+".";
	int_to_str.clear();
	int_to_str<<1900+ltm->tm_year;
	int_to_str>>year;
	ret+=year;
	int_to_str.clear();
	return ret;
};
string validate_date_format(string str){
	regex reg("(-|:|;|,|/)");
	str = regex_replace(str,reg,".");
	reg.assign("(\\b0)");
	str = regex_replace(str,reg,"");
	return str;
};

/* functions to create sql queries */
string create_table_query(){
	return"CREATE TABLE IF NOT EXISTS TRAINING(\
	       record INTEGER PRIMARY KEY AUTOINCREMENT,\
	       day TEXT NOT NULL,\
	       exercise_name TEXT NOT NULL,\
	       weight INT NOT NULL,\
	       series INT NOT NULL,\
	       reps INT NOT NULL);";
};

string basic_select(string day){
	embed(day);
	return "SELECT exercise_name \"exercise name\",weight,series,\
	      	reps, weight*series*reps as load\
	      	FROM TRAINING WHERE day = "+day+";";
};
string simple_select(string day){
	embed(day);
	return "SELECT distinct exercise_name \"exercise name\"\
		FROM TRAINING WHERE day = "+day+";";
};
string select_total_load(string day){
	embed(day);
	return "SELECT sum(weight*series*reps) \"total load\"\
	        FROM TRAINING WHERE day="+day+";";
};
string format_insertion_query(string date, string exercise_name, int weight, int series, int reps){
	int_to_str.str(string()); //clearing stringstream
	string query="";
	embed(date);
	embed(exercise_name);
	query="INSERT INTO TRAINING(day,exercise_name,weight,series,reps) VALUES(";
	int_to_str<<date<<", ";
	int_to_str<<exercise_name<<", ";
	int_to_str<<weight<<", ";
	int_to_str<<series<<", ";
	int_to_str<<reps;
	query.append(int_to_str.str());
	query.append(");");
	return query;
};

int menu(){
	int choice;
	cout<<"1.insert a new training"<<endl;
	cout<<"2.print the data about the previous training"<<endl;
	cout<<"3.print the data about trainings in the specified time-frame"<<endl;
	cout<<"enter the number: ";
	cin>>choice;
	if(choice<1&&choice>3){
		cout<<"invalid input"<<endl;
		exit(1);
	}
	else
		return choice;
};
/* function for case1 */
void add_new_training(){
	string exercise_name;
	int weight,series,reps;
	cout<<"enter the exercise name: "<<endl;
	cin.ignore(); //debugging getline
	getline(cin,exercise_name);
	cout<<"enter the weight: "<<endl;
	cin>>weight;
	cout<<"enter the number of series: ";
	cin>>series;
	cout<<"enter the number of reps: ";
	cin>>reps;
	string query=format_insertion_query(format_date(),exercise_name,weight,series,reps);
	rc=sqlite3_exec(db, query.c_str(), callback, 0, &err_msg);
};
/* function for case2 */
void display_data_for_day(){
	string input;
	cout<<"which training day would you like to print info about: ";
	cin>>input;
	string day;

	if(strcasecmp(input.c_str(),"today") == 0){
		day=format_date();
	}
	else if(strcasecmp(input.c_str(),"yesterday") == 0){
		day=format_date(ltm->tm_mday-1);
	}
	
	else{
		day=validate_date_format(input);
	}
	string query=basic_select(day);
	cout<<endl;
	cout<<day<<endl;
	rc=sqlite3_exec(db, query.c_str(), callback, 0, &err_msg);
	query=select_total_load(day);
	rc=sqlite3_exec(db, query.c_str(), callback, 0, &err_msg);
};
/* function for case3 */
void display_data_for_period(){
	string input, first_day, last_day;
	bool short_display=false;
	cout<<"enter the first date of period: ";
	cin.ignore();
	getline(cin,input);

	//setting flag for short display
	if(input.find("-s")!=string::npos){
		short_display=true;
		input.pop_back();
		input.pop_back();
		input.pop_back();
		//cout<<input<<endl;
	}

	if(strcasecmp(input.c_str(),"week") == 0){
		last_day=format_date();
		first_day=format_date(ltm->tm_mday-6);
	}
	else{
	first_day = validate_date_format(input);
	cout<<"enter the last day of period: ";
	cin>>input;
	last_day=validate_date_format(input);
	}
	cout<<endl;
	regex reg("\\b[0-9]{1,2}");
	smatch m;
	regex_search(first_day,m,reg);
	int day_first,day_last;
	sscanf(m.str().c_str(),"%d",&day_first);
	regex_search(last_day,m,reg);
	sscanf(m.str().c_str(),"%d",&day_last);

	if(short_display){
		for(int i=day_first;i<=day_last;i++){
			string day =format_date(i);
			cout<<day<<endl;
			rc=sqlite3_exec(db, simple_select(day).c_str(),callback, 0, &err_msg);
		}
	}
	else{
		for(int i=day_first;i<=day_last;i++){
			string day =format_date(i);
			cout<<day<<endl;
			rc=sqlite3_exec(db, basic_select(day).c_str(),callback, 0, &err_msg);
			rc=sqlite3_exec(db, select_total_load(day).c_str(),callback, 0, &err_msg);
		}
	}
};

int main(int argc, char **argv){

	/* Open database */
	rc = sqlite3_open("database.db", &db);
	if(rc)
	{
		cout<<"cannot open the database: "<<sqlite3_errmsg(db) <<endl;
		exit(1);
	}
	else 
		cout<<"successfully established the database connection "<<endl;

	/* ensure that table was created correctly*/
	sqlite3_exec(db, create_table_query().c_str(),callback, 0, &err_msg);

	/* main flow */
	switch(menu()){
	case 1: //ADD NEW TRAINING
		{
		add_new_training();
		break;
		}
	case 2: //DISPLAY TRAINING DATA FOR EXACT DAY
		{
		display_data_for_day();
		break;
		}
	case 3: //DISPLAY TRAINING DATA FOR PERIOD OF TIME
		{
		display_data_for_period();
		break;
		}
	}

	/* checking what happened */
	if (rc != SQLITE_OK){
		printf("SQL error: %s\n",err_msg);
		sqlite3_free(err_msg);
	}else{
		fprintf(stdout,"query executed successfully\n");
	}
	sqlite3_close(db);
	return 0;
}
