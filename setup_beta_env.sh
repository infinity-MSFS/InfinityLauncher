#!/bin/bash

set -e

if [ "$(id -u)" -ne 0 ]; then
  echo "This script must be run as root. Please use sudo."
  exit 1
fi

echo "Creating motherboard info service..."

cat >/etc/systemd/system/motherboard-info.service <<'EOF'
[Unit]
Description=Extract motherboard information for applications
After=local-fs.target

[Service]
Type=oneshot
ExecStart=/bin/sh -c "dmidecode -s baseboard-serial-number > /var/run/motherboard-serial"
ExecStartPost=/bin/chmod 644 /var/run/motherboard-serial
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

if ! command -v dmidecode &>/dev/null; then
  echo "dmidecode not found. Installing..."

  if command -v apt-get &>/dev/null; then
    apt-get update && apt-get install -y dmidecode
  elif command -v dnf &>/dev/null; then
    dnf install -y dmidecode
  elif command -v yum &>/dev/null; then
    yum install -y dmidecode
  elif command -v pacman &>/dev/null; then
    pacman -S --noconfirm dmidecode
  else
    echo "Unsupported package manager. Please install dmidecode manually."
    exit 1
  fi
fi

systemctl daemon-reload

systemctl enable motherboard-info.service
systemctl start motherboard-info.service

if systemctl is-active --quiet motherboard-info.service && [ -f /var/run/motherboard-serial ]; then
  echo "Service installed and running successfully."
  echo "Motherboard serial is now available at /var/run/motherboard-serial"
  echo "Serial: $(cat /var/run/motherboard-serial)"
else
  echo "Service installation failed or service is not running."
  exit 1
fi

echo "Setup complete! You may now run the Infinity Launcher"
