const http = require('http');

// Options for the HTTP request
const options = {
    hostname: 'localhost',
    port: 6435,
    path: '/read',
    method: 'POST',

};



    // Create another request on the same persistent connection
    const req2 = http.request(options, (res) => {
        
        let data = '';
        
        // Data chunk received from the server
        res.on('data', (chunk) => {
            data = chunk;
        });
        
        // Response has been completely received
        res.on('end', () => {
            let xp = data.toString('utf8');
            console.log(xp)
            
          
            
        });
    });

    // Handle any errors on the second request
    req2.on('error', (e) => {
        console.error(`Error making request: ${e.message}`);
    });

    // Now we are requesting the actual data
    console.time('Execution Time');
    req2.end();  // Sending the read request here
    console.timeEnd('Execution Time');
    

