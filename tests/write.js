const http = require('http');

// Data to be sent in the write request
const postData = JSON.stringify({
    name: 'tommy',
    value: 'I want you forever'
});

// Options for the HTTP request
const options = {
    hostname: 'localhost',
    port: 6436,
    path: '/write',  // Adjust the path if necessary
    method: 'POST',
    headers: {
        'Content-Length': Buffer.byteLength(postData)
    }
};

// Create the request
const req = http.request(options, (res) => {
    let data = '';

    // Data chunk received from the server
    res.on('data', (chunk) => {
        data = chunk;
        console.log(data)
    });


});

// Handle any errors on the request
req.on('error', (e) => {
    console.error(`Error making request: ${e.message}`);
});

// Now we are sending the actual data
console.time('Execution Time');
req.write(postData);  // Sending the write request here
req.end();            // Close the request
console.timeEnd('Execution Time');
