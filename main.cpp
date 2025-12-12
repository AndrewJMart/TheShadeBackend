#include<stdio.h>
#include<string.h>
#include<crow.h>
#include<sqlite3.h>
#include<mutex>

std::mutex db_mutex;

int main()
{
	sqlite3 *db;
	sqlite3_stmt* stmt;
	int rc;

	// Insert Into DB
	rc = sqlite3_open("./emails.db", &db);

	if (rc) {
		fprintf(stderr, "Cannot Open DB Connection");
		exit(1);
	}

	// Create Emails Table
	const char *email_table_sql = "CREATE TABLE IF NOT EXISTS emails (id INTEGER PRIMARY KEY AUTOINCREMENT, email VARCHAR(100) NOT NULL);";

	rc = sqlite3_prepare_v2(db, email_table_sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot Prepare Creation Of Table");
		exit(1);
	}

	// Execute Table Creation SQL
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	
	// Close Initial Connection
	sqlite3_close(db);

	crow::SimpleApp app;

	CROW_ROUTE(app, "/newsletter")
		.methods("POST"_method)
		([](const crow::request& req){

		// Parse JSON For String
		auto x = crow::json::load(req.body);

		// Check If Valid JSON
		if (!x) 
			return crow::response(crow::status::BAD_REQUEST);

		// Extract Email From JSON
		std::string user_email = x["email"].s(); 

		// Prepare SQLite Query
		sqlite3 *insert_db;
		sqlite3_stmt* insert_stmt;
		int rc;

		// Concurrency Lock (Will Unlock Once Out Of Scope)
		std::lock_guard<std::mutex> lock(db_mutex);

		// Open DB
		rc = sqlite3_open("./emails.db", &insert_db);

		if (rc)
			return crow::response("501", "Failed To Open Database"); // Return Internal Server Error

		
		const char *insert_email = "INSERT INTO emails (email) VALUES (?)";
		rc = sqlite3_prepare_v2(insert_db, insert_email, -1, &insert_stmt, NULL);

		if (rc != SQLITE_OK) {
			// Free Statement & DB
			sqlite3_finalize(insert_stmt);
			sqlite3_close(insert_db);	

			// Return Internal Server Error
			return crow::response("501", "Failed To Prepare Insertion");
		}

		// Bind, Execute, And Close SQL Statement & DB Connection
		sqlite3_bind_text(insert_stmt, 1, user_email.c_str(), user_email.length(), SQLITE_TRANSIENT);
		sqlite3_step(insert_stmt);
		sqlite3_finalize(insert_stmt);
		sqlite3_close(insert_db);

		return crow::response("200", "Email Inserted\n");
	});

	app.port(928).multithreaded().run();
}
