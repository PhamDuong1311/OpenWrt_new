# Netifd (Network Interface Daemon)

## What is netifd ?
`netifd` is an **RPC-cable daemon** written in C for better access to kernel APIs with the ability to listen on **netlink events**.

## Why do we want netifd ?
One thing that `netifd` does much better then **old OpenWrt-network configuration scripts** is handling configuration changes. With `netifd`, when the file `/etc/config/network` changes, you no longer have to restart all interfaces. Simply run `/etc/init.d/network reload`. This will issue an `ubus-call` to `netifd`, telling it to figure out the difference between runtime state and the new config, then apply only that.