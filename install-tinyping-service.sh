#!/bin/bash
# Run from the directory containing the compiled tinyping executable:
# sudo bash install-tinyping-service.sh
set -euo pipefail

if [[ "$EUID" -ne 0 ]]; then
    echo "Run this script with: sudo bash install-tinyping-service.sh" >&2
    exit 1
fi

if ! command -v systemctl >/dev/null 2>&1 || [[ ! -d /run/systemd/system ]]; then
    echo "This script requires Linux with systemd running." >&2
    exit 1
fi

if [[ ! -f ./tinyping ]]; then
    echo "Compiled tinyping executable not found. Run this from your project directory." >&2
    exit 1
fi

# Stop an existing service before replacing its executable.
if systemctl is-active --quiet tinyping.service; then
    systemctl stop tinyping.service
fi

echo "Installing TinyPing..."
install -D -o root -g root -m 755 ./tinyping /usr/local/bin/tinyping
mkdir -p /var/opt/tinyping

# This creates or replaces the unit without changing TinyPing's configuration.
echo "Creating tinyping.service..."
cat > /etc/systemd/system/tinyping.service <<'EOF'
[Unit]
Description=TinyPing uptime monitor
Wants=network-online.target
After=network-online.target

[Service]
Type=simple
User=root
ExecStart=/usr/local/bin/tinyping
WorkingDirectory=/var/opt/tinyping
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
chmod 644 /etc/systemd/system/tinyping.service

echo "Enabling and starting TinyPing..."
systemctl daemon-reload
systemctl enable --now tinyping.service

# Show status without opening an interactive pager.
systemctl status tinyping.service --no-pager
