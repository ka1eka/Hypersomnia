server {
    server_name hypersomnia.io www.hypersomnia.io;

    root /var/www/html;
    index Hypersomnia.html;

    location / {
        # Try to serve file directly, then directories, then fall back to Hypersomnia.html
        try_files $uri $uri/ /Hypersomnia.html;
    }

    # MIME types for WebAssembly (.wasm) files
    types {
        application/wasm     wasm;
        text/html            html;
        text/javascript      js;
    }

    # Add Cross-Origin-Opener-Policy and Cross-Origin-Embedder-Policy headers
    add_header Cross-Origin-Resource-Policy "same-site";
    add_header Cross-Origin-Opener-Policy "same-origin";
    add_header Cross-Origin-Embedder-Policy "require-corp";

    listen 443 ssl; # managed by Certbot
    ssl_certificate /etc/letsencrypt/live/hypersomnia.io/fullchain.pem; # managed by Certbot
    ssl_certificate_key /etc/letsencrypt/live/hypersomnia.io/privkey.pem; # managed by Certbot
    include /etc/letsencrypt/options-ssl-nginx.conf; # managed by Certbot
    ssl_dhparam /etc/letsencrypt/ssl-dhparams.pem; # managed by Certbot
}

# HTTP server block for redirecting to HTTPS
server {
    if ($host = www.hypersomnia.io) {
        return 301 https://$host$request_uri;
    } # managed by Certbot

    if ($host = hypersomnia.io) {
        return 301 https://$host$request_uri;
    } # managed by Certbot

    listen 80;
    server_name hypersomnia.io www.hypersomnia.io;
    return 404; # managed by Certbot
}