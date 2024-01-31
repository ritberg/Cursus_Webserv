#!/usr/bin/python
# -*- coding: utf-8 -*- 

import cgi
import os

# HTML content
print("<html><head><title>Photo Upload CGI</title></head><body>")
print("<h1>Photo uploading</h1>")

# Retrieve user input from the form
form = cgi.FieldStorage()

if os.environ.get('REQUEST_METHOD', '') == 'POST':

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
