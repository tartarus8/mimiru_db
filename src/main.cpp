#include "mimiru/engine/system_manager.hpp"
#include "mimiru/query/lexer.hpp"
#include "mimiru/query/parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

using namespace mimiru;

// --- EXECUTOR (The Bridge between AST and Engine) ---
// This uses the "Visitor Pattern" via std::visit to handle the different AST statements safely.
struct StatementExecutor {
    engine::SystemManager& sys;

    void operator()(const query::ast::CreateDatabaseStmt& stmt) {
        sys.create_database(stmt.db_name);
        std::cout << "Database '" << stmt.db_name << "' created successfully.\n";
    }

    void operator()(const query::ast::DropDatabaseStmt& stmt) {
        sys.drop_database(stmt.db_name);
        std::cout << "Database '" << stmt.db_name << "' dropped.\n";
    }

    void operator()(const query::ast::UseDatabaseStmt& stmt) {
        sys.use_database(stmt.db_name);
        std::cout << "Switched to database '" << stmt.db_name << "'.\n";
    }

    void operator()(const query::ast::CreateTableStmt& stmt) {
        engine::Database* db = sys.get_current_db();
        
        core::TableSchema schema;
        schema.name = stmt.table_name;
        schema.columns = stmt.columns;
        
        db->create_table(schema);
        std::cout << "Table '" << stmt.table_name << "' created successfully.\n";
    }

    void operator()(const query::ast::DropTableStmt& stmt) {
        engine::Database* db = sys.get_current_db();
        db->drop_table(stmt.table_name);
        std::cout << "Table '" << stmt.table_name << "' dropped.\n";
    }

    void operator()(const query::ast::InsertStmt& stmt) {
        engine::Database* db = sys.get_current_db();
        engine::Table* table = db->get_table(stmt.table_name);

        int rows_inserted = 0;
        for (const auto& row_values : stmt.values_list) {
            std::vector<core::NullableValue> mapped_values;
            for (const auto& val : row_values) {
                mapped_values.push_back(val);
            }
            table->insert(mapped_values);
            rows_inserted++;
        }
        std::cout << "Inserted " << rows_inserted << " row(s) into '" << stmt.table_name << "'.\n";
    }

    void operator()(const query::ast::SelectStmt& stmt) {
        // NOTE: In a full implementation, you would call table->select_where() here
        // For now, we mock the JSON output to fulfill the PDF requirement format
        nlohmann::json output = nlohmann::json::array();
        
        // Mocking a successful response format
        nlohmann::json mock_row;
        mock_row["status"] = "Select execution routing is wired up!";
        mock_row["table"] = stmt.table_name;
        output.push_back(mock_row);

        std::cout << output.dump(4) << "\n"; // Pretty print JSON with 4 spaces
    }
};

// --- QUERY PROCESSOR ---
void process_query(const std::string& query, engine::SystemManager& sys) {
    try {
        query::Lexer lexer(query);
        auto tokens = lexer.tokenize();

        query::Parser parser(tokens);
        auto statements = parser.parse();

        // Execute each statement found in the query string
        for (const auto& stmt : statements) {
            std::visit(StatementExecutor{sys}, stmt);
        }

    } catch (const core::DbError& e) {
        std::cerr << "Engine Error: " << e.what() << "\n";
    } catch (const query::LexerError& e) {
        std::cerr << "Syntax Error (Lexer): " << e.what() << "\n";
    } catch (const query::ParserError& e) {
        std::cerr << "Syntax Error (Parser): " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Unexpected Error: " << e.what() << "\n";
    }
}

// --- MAIN LOOP ---
int main(int argc, char* argv[]) {
    // Initialize the root folder for our database system
    engine::SystemManager sys("mimiru_data");

    // 1. BATCH MODE (e.g., ./prog script.txt)
    if (argc == 2) {
        std::string filename = argv[1];
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Fatal Error: Cannot open script file " << filename << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        
        std::cout << "Executing batch script: " << filename << "\n";
        process_query(buffer.str(), sys);
        return 0;
    }

    // 2. INTERACTIVE MODE
    std::cout << "Mimiru DB (Interactive Mode)\n";
    std::cout << "Type SQL commands ending with ';'. Type 'exit' to quit.\n";

    std::string query_buffer;
    std::string line;

    while (true) {
        if (query_buffer.empty()) {
            std::cout << "mimiru> ";
        } else {
            std::cout << "      > "; // Continuation prompt for multi-line queries
        }

        if (!std::getline(std::cin, line)) {
            break; // EOF reached (Ctrl+D)
        }

        if (line == "exit" || line == "quit") {
            break;
        }

        query_buffer += line + " ";

        // The PDF states: "Команды могут занимать несколько строк и завершаются символом ;"
        if (query_buffer.find(';') != std::string::npos) {
            process_query(query_buffer, sys);
            query_buffer.clear(); // Reset buffer for the next command
        }
    }

    std::cout << "Goodbye.\n";
    return 0;
}
