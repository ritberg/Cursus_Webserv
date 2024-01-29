#!/usr/bin/python

# HTML content
print("<html><head><title>Greeting CGI</title></head><body>")
print("<h1>Personalized Greeting</h1>")

# Retrieve user input from the form
import cgi

form = cgi.FieldStorage()
user_name = form.getvalue("name")


# Process the input and generate a response
if user_name:
    print("<p>Hello, {}!</p>".format(user_name))
else:
    print("<p>Please enter your name in the form below.</p>")

# HTML form for user input
print("<form action='' method='post'>")
print("  <label for='name'>Your Name:</label>")
print("  <input type='text' name='name'>")
print("  <input type='submit' value='Submit'>")
print("</form>")

print("</body></html>")

