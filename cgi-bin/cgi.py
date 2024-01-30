#!/usr/bin/python

import cgi
import os

# HTML content
print("<html><head><title>Photo Upload CGI</title></head><body>")
print("<h1>Photo uploading</h1>")

# Retrieve user input from the form
form = cgi.FieldStorage()
user_name = form.getvalue("name")

if os.environ.get('REQUEST_METHOD', '') == 'POST':
    # Process the input and generate a response
    if user_name:
        print("<p>Hello, {}!</p>".format(user_name))
    else:
        print("<p>Please enter your name in the form below.</p>")

    # HTML form for user input with a file upload field
    print("<form action='' method='post' enctype='multipart/form-data'>")
    print("  <label for='name'>Your Name:</label>")
    print("  <input type='text' name='name'>")
    print("  <br>")
    print("  <label for='photo'>Choose a photo:</label>")
    print("  <input type='file' name='photo' accept='image/*' required>")
    print("  <br>")
    print("  <input type='submit' value='Upload'>")
    print("</form>")

print("</body></html>")



# #include <iostream>
# #include <string>

# int main()
# {
#     std::cout << "Content-type: text/html\n\n";
#     std::cout << "<html><head><title>Greeting CGI</title></head><body>\n";
#     std::cout << "<h1>Personalized Greeting</h1>\n";

#     // Retrieve user input from the form
#     std::string user_name;
#     char *data = getenv("QUERY_STRING");
#     if (data != NULL)
#     {
#         size_t pos = std::string(data).find("name=");
#         if (pos != std::string::npos)
#             user_name = std::string(data).substr(pos + 5);
#     }

#     // Process the input and generate a response
#     if (!user_name.empty())
#         std::cout << "<p>Hello, " << user_name << "!</p>\n";
#     else
#         std::cout << "<p>Please enter your name in the form below.</p>\n";

#     // HTML form for user input
#     std::cout << "<form action='' method='get'>\n";
#     std::cout << "  <label for='name'>Your Name:</label>\n";
#     std::cout << "  <input type='text' name='name'>\n";
#     std::cout << "  <input type='submit' value='Submit'>\n";
#     std::cout << "</form>\n";

#     std::cout << "</body></html>\n";

#     return 0;
# }
