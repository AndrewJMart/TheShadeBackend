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

	// Connect To DB
	const char *db_filename = "./emails.db";

	rc = sqlite3_open(db_filename, &db);

	if (rc) {
		fprintf(stderr, "Cannot Open DB Connection");
		exit(1);
	}

	// Create Emails Table
	const char *email_table_sql = "CREATE TABLE IF NOT EXISTS emails (id INT AUTO_INCREMENT PRIMARY KEY, email VARCHAR(100) NOT NULL);";

	rc = sqlite3_prepare_v2(db, email_table_sql, -1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot Prepare Creation Of Table");
		exit(1);
	}

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

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

		// Concurrency Lock (Will Unlock Once Out Of Scope)
		std::lock_guard<std::mutex> lock(db_mutex);

		// 

		return crow::response("200", "STRING FOUND");
	});

	app.port(928).multithreaded().run();
}

