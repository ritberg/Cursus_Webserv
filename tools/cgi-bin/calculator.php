<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Simple Calculator</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 50px;
        }

        input, select, button {
            margin: 10px;
            font-size: 16px;
        }
    </style>
</head>
<body>

<form method="post" action="" onsubmit="calculateResult(event)">
    <input type="number" name="num1" placeholder="Enter number 1" required>
    <select name="operator" required>
        <option value="add">+</option>
        <option value="subtract">-</option>
        <option value="multiply">*</option>
        <option value="divide">/</option>
    </select>
    <input type="number" name="num2" placeholder="Enter number 2" required>
    <button type="submit" name="submit" value="">Calculate</button>
</form>

<div id="resultContainer"></div>

<script>
    function calculateResult(event) {
        event.preventDefault(); // Prevent form submission, as we'll handle it with JavaScript

        // Fetch form data
        var num1 = parseFloat(document.querySelector('input[name="num1"]').value);
        var operator = document.querySelector('select[name="operator"]').value;
        var num2 = parseFloat(document.querySelector('input[name="num2"]').value);

        // Perform the calculation
        var result;
        switch (operator) {
            case 'add':
                result = num1 + num2;
                break;
            case 'subtract':
                result = num1 - num2;
                break;
            case 'multiply':
                result = num1 * num2;
                break;
            case 'divide':
                // Handle division by zero
                result = (num2 !== 0) ? num1 / num2 : 'Cannot divide by zero';
                break;
            default:
                result = 'Invalid operator';
        }

        // Display the result
        var resultContainer = document.getElementById('resultContainer');
        resultContainer.innerHTML = '<h2>Result: ' + result + '</h2>';
    }
</script>

</body>
</html>
