/**
 * Arduino Communication Library
 * JavaScript library for communicating with Arduino controllers
 * Smart Room Reservation System
 */

class ArduinoController {
    constructor(apiBaseUrl = '') {
        this.apiBaseUrl = apiBaseUrl || this.getApiBaseUrl();
        this.roomId = null;
        this.pollInterval = null;
        this.statusCallbacks = [];
        this.errorCallbacks = [];
    }

    /**
     * Get API base URL from current environment
     */
    getApiBaseUrl() {
        if (typeof API_BASE_URL !== 'undefined') {
            return API_BASE_URL;
        }
        return window.location.origin;
    }

    /**
     * Set the room ID for this controller
     * @param {number} roomId - Room ID to control
     */
    setRoomId(roomId) {
        this.roomId = roomId;
        console.log(`🔧 Arduino Controller set to Room ID: ${roomId}`);
    }

    /**
     * Get current room status from Arduino
     * @returns {Promise<Object>} Room status data
     */
    async getRoomStatus() {
        if (!this.roomId) {
            throw new Error('Room ID not set. Call setRoomId() first.');
        }

        try {
            console.log(`📡 Fetching Arduino status for room ${this.roomId}`);
            
            const response = await fetch(`${this.apiBaseUrl}/api/arduino/status/${this.roomId}`, {
                method: 'GET',
                headers: {
                    'Content-Type': 'application/json'
                }
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const data = await response.json();
            console.log('✅ Arduino status received:', data);
            
            // Trigger status callbacks
            this.statusCallbacks.forEach(callback => {
                try {
                    callback(data);
                } catch (error) {
                    console.error('❌ Error in status callback:', error);
                }
            });

            return data;
        } catch (error) {
            console.error('❌ Failed to get room status:', error);
            this.triggerErrorCallbacks(error);
            throw error;
        }
    }

    /**
     * Send unlock command to Arduino
     * @param {string} accessCode - Access code for the room
     * @param {string} userId - User ID (optional)
     * @returns {Promise<Object>} Unlock response
     */
    async unlockRoom(accessCode, userId = null) {
        if (!this.roomId) {
            throw new Error('Room ID not set. Call setRoomId() first.');
        }

        if (!accessCode || accessCode.length < 4) {
            throw new Error('Access code is required and must be at least 4 characters');
        }

        try {
            console.log(`🔓 Sending unlock command for room ${this.roomId}`);
            
            const payload = {
                accessCode: accessCode,
                userId: userId || `web_user_${Date.now()}`
            };

            const response = await fetch(`${this.apiBaseUrl}/api/arduino/unlock/${this.roomId}`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(payload)
            });

            const data = await response.json();

            if (!response.ok) {
                console.error('❌ Unlock failed:', data);
                throw new Error(data.error || `HTTP error! status: ${response.status}`);
            }

            console.log('✅ Room unlocked successfully:', data);
            return data;
        } catch (error) {
            console.error('❌ Failed to unlock room:', error);
            this.triggerErrorCallbacks(error);
            throw error;
        }
    }

    /**
     * Send check-in command to Arduino
     * @returns {Promise<Object>} Check-in response
     */
    async checkIn() {
        if (!this.roomId) {
            throw new Error('Room ID not set. Call setRoomId() first.');
        }

        try {
            console.log(`📍 Checking in to room ${this.roomId}`);
            
            const response = await fetch(`${this.apiBaseUrl}/api/arduino/checkin/${this.roomId}`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({})
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const data = await response.json();
            console.log('✅ Checked in successfully:', data);
            return data;
        } catch (error) {
            console.error('❌ Failed to check in:', error);
            this.triggerErrorCallbacks(error);
            throw error;
        }
    }

    /**
     * Send check-out command to Arduino
     * @returns {Promise<Object>} Check-out response
     */
    async checkOut() {
        if (!this.roomId) {
            throw new Error('Room ID not set. Call setRoomId() first.');
        }

        try {
            console.log(`📤 Checking out of room ${this.roomId}`);
            
            const response = await fetch(`${this.apiBaseUrl}/api/arduino/checkout/${this.roomId}`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({})
            });

            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }

            const data = await response.json();
            console.log('✅ Checked out successfully:', data);
            return data;
        } catch (error) {
            console.error('❌ Failed to check out:', error);
            this.triggerErrorCallbacks(error);
            throw error;
        }
    }

    /**
     * Start polling room status at regular intervals
     * @param {number} intervalMs - Polling interval in milliseconds (default: 30000)
     */
    startStatusPolling(intervalMs = 30000) {
        if (this.pollInterval) {
            console.log('⚠️  Status polling already started');
            return;
        }

        console.log(`🔄 Starting status polling every ${intervalMs / 1000} seconds`);
        
        this.pollInterval = setInterval(() => {
            this.getRoomStatus().catch(error => {
                console.error('❌ Polling error:', error);
            });
        }, intervalMs);

        // Initial status check
        this.getRoomStatus().catch(error => {
            console.error('❌ Initial status check error:', error);
        });
    }

    /**
     * Stop polling room status
     */
    stopStatusPolling() {
        if (this.pollInterval) {
            clearInterval(this.pollInterval);
            this.pollInterval = null;
            console.log('🛑 Status polling stopped');
        }
    }

    /**
     * Add callback for status updates
     * @param {Function} callback - Function to call when status updates
     */
    onStatusUpdate(callback) {
        if (typeof callback === 'function') {
            this.statusCallbacks.push(callback);
            console.log('📋 Status callback registered');
        } else {
            console.error('❌ Status callback must be a function');
        }
    }

    /**
     * Add callback for errors
     * @param {Function} callback - Function to call when errors occur
     */
    onError(callback) {
        if (typeof callback === 'function') {
            this.errorCallbacks.push(callback);
            console.log('📋 Error callback registered');
        } else {
            console.error('❌ Error callback must be a function');
        }
    }

    /**
     * Remove status callback
     * @param {Function} callback - Callback function to remove
     */
    removeStatusCallback(callback) {
        const index = this.statusCallbacks.indexOf(callback);
        if (index > -1) {
            this.statusCallbacks.splice(index, 1);
            console.log('🗑️  Status callback removed');
        }
    }

    /**
     * Remove error callback
     * @param {Function} callback - Callback function to remove
     */
    removeErrorCallback(callback) {
        const index = this.errorCallbacks.indexOf(callback);
        if (index > -1) {
            this.errorCallbacks.splice(index, 1);
            console.log('🗑️  Error callback removed');
        }
    }

    /**
     * Trigger error callbacks
     * @private
     */
    triggerErrorCallbacks(error) {
        this.errorCallbacks.forEach(callback => {
            try {
                callback(error);
            } catch (callbackError) {
                console.error('❌ Error in error callback:', callbackError);
            }
        });
    }

    /**
     * Create room control widget
     * @param {HTMLElement} container - Container element for the widget
     * @param {Object} options - Widget options
     */
    createControlWidget(container, options = {}) {
        const {
            showStatus = true,
            showUnlock = true,
            showCheckInOut = true,
            theme = 'default'
        } = options;

        const widget = document.createElement('div');
        widget.className = `arduino-control-widget theme-${theme}`;
        widget.innerHTML = `
            <div class="widget-header">
                <h3>🏠 Room ${this.roomId} Control</h3>
                <div class="status-indicator" id="status-indicator">⚪</div>
            </div>
            
            ${showStatus ? `
                <div class="status-section">
                    <div class="status-display" id="status-display">
                        <div class="status-item">
                            <span class="label">Status:</span>
                            <span class="value" id="room-status">Unknown</span>
                        </div>
                        <div class="status-item">
                            <span class="label">Occupied:</span>
                            <span class="value" id="room-occupied">Unknown</span>
                        </div>
                        <div class="status-item">
                            <span class="label">Last Update:</span>
                            <span class="value" id="last-update">Never</span>
                        </div>
                    </div>
                    <button class="btn-refresh" id="btn-refresh">🔄 Refresh</button>
                </div>
            ` : ''}
            
            ${showUnlock ? `
                <div class="unlock-section">
                    <div class="input-group">
                        <input type="password" id="access-code" placeholder="Enter access code" maxlength="10">
                        <button class="btn-unlock" id="btn-unlock">🔓 Unlock</button>
                    </div>
                </div>
            ` : ''}
            
            ${showCheckInOut ? `
                <div class="checkinout-section">
                    <button class="btn-checkin" id="btn-checkin">📍 Check In</button>
                    <button class="btn-checkout" id="btn-checkout">📤 Check Out</button>
                </div>
            ` : ''}
            
            <div class="message-area" id="message-area"></div>
        `;

        // Add CSS styles
        const style = document.createElement('style');
        style.textContent = `
            .arduino-control-widget {
                border: 1px solid #ddd;
                border-radius: 8px;
                padding: 16px;
                background: #f9f9f9;
                font-family: Arial, sans-serif;
                max-width: 400px;
                margin: 16px 0;
            }
            .widget-header {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 16px;
                border-bottom: 1px solid #eee;
                padding-bottom: 8px;
            }
            .widget-header h3 {
                margin: 0;
                color: #333;
            }
            .status-indicator {
                font-size: 20px;
            }
            .status-section, .unlock-section, .checkinout-section {
                margin-bottom: 16px;
            }
            .status-display {
                background: white;
                border: 1px solid #eee;
                border-radius: 4px;
                padding: 12px;
                margin-bottom: 8px;
            }
            .status-item {
                display: flex;
                justify-content: space-between;
                margin-bottom: 4px;
            }
            .status-item .label {
                font-weight: bold;
                color: #666;
            }
            .input-group {
                display: flex;
                gap: 8px;
            }
            .input-group input {
                flex: 1;
                padding: 8px;
                border: 1px solid #ddd;
                border-radius: 4px;
            }
            .arduino-control-widget button {
                padding: 8px 16px;
                border: none;
                border-radius: 4px;
                cursor: pointer;
                font-size: 14px;
                transition: all 0.2s;
            }
            .btn-refresh, .btn-checkin {
                background: #007bff;
                color: white;
            }
            .btn-unlock {
                background: #28a745;
                color: white;
            }
            .btn-checkout {
                background: #dc3545;
                color: white;
            }
            .arduino-control-widget button:hover {
                opacity: 0.8;
                transform: translateY(-1px);
            }
            .arduino-control-widget button:disabled {
                opacity: 0.5;
                cursor: not-allowed;
            }
            .message-area {
                margin-top: 16px;
                padding: 8px;
                border-radius: 4px;
                min-height: 20px;
                font-size: 14px;
            }
            .message-success {
                background: #d4edda;
                color: #155724;
                border: 1px solid #c3e6cb;
            }
            .message-error {
                background: #f8d7da;
                color: #721c24;
                border: 1px solid #f5c6cb;
            }
            .message-info {
                background: #d1ecf1;
                color: #0c5460;
                border: 1px solid #bee5eb;
            }
            .checkinout-section {
                display: flex;
                gap: 8px;
            }
            .checkinout-section button {
                flex: 1;
            }
        `;
        
        if (!document.querySelector('#arduino-widget-styles')) {
            style.id = 'arduino-widget-styles';
            document.head.appendChild(style);
        }

        container.appendChild(widget);

        // Bind event listeners
        this.bindWidgetEvents(widget);

        // Initial status update
        this.getRoomStatus().then(status => {
            this.updateWidgetStatus(widget, status);
        }).catch(error => {
            this.showWidgetMessage(widget, 'Failed to load status', 'error');
        });

        return widget;
    }

    /**
     * Bind event listeners to widget
     * @private
     */
    bindWidgetEvents(widget) {
        const refreshBtn = widget.querySelector('#btn-refresh');
        const unlockBtn = widget.querySelector('#btn-unlock');
        const checkinBtn = widget.querySelector('#btn-checkin');
        const checkoutBtn = widget.querySelector('#btn-checkout');
        const accessCodeInput = widget.querySelector('#access-code');

        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => {
                this.getRoomStatus().then(status => {
                    this.updateWidgetStatus(widget, status);
                    this.showWidgetMessage(widget, 'Status updated', 'success');
                }).catch(error => {
                    this.showWidgetMessage(widget, 'Failed to refresh status', 'error');
                });
            });
        }

        if (unlockBtn && accessCodeInput) {
            const doUnlock = () => {
                const accessCode = accessCodeInput.value.trim();
                if (!accessCode) {
                    this.showWidgetMessage(widget, 'Please enter access code', 'error');
                    return;
                }

                unlockBtn.disabled = true;
                this.unlockRoom(accessCode).then(result => {
                    this.showWidgetMessage(widget, 'Room unlocked successfully!', 'success');
                    accessCodeInput.value = '';
                    // Refresh status after unlock
                    setTimeout(() => {
                        this.getRoomStatus().then(status => {
                            this.updateWidgetStatus(widget, status);
                        });
                    }, 1000);
                }).catch(error => {
                    this.showWidgetMessage(widget, error.message || 'Unlock failed', 'error');
                }).finally(() => {
                    unlockBtn.disabled = false;
                });
            };

            unlockBtn.addEventListener('click', doUnlock);
            accessCodeInput.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') {
                    doUnlock();
                }
            });
        }

        if (checkinBtn) {
            checkinBtn.addEventListener('click', () => {
                checkinBtn.disabled = true;
                this.checkIn().then(result => {
                    this.showWidgetMessage(widget, 'Checked in successfully!', 'success');
                    // Refresh status after checkin
                    setTimeout(() => {
                        this.getRoomStatus().then(status => {
                            this.updateWidgetStatus(widget, status);
                        });
                    }, 1000);
                }).catch(error => {
                    this.showWidgetMessage(widget, error.message || 'Check-in failed', 'error');
                }).finally(() => {
                    checkinBtn.disabled = false;
                });
            });
        }

        if (checkoutBtn) {
            checkoutBtn.addEventListener('click', () => {
                checkoutBtn.disabled = true;
                this.checkOut().then(result => {
                    this.showWidgetMessage(widget, 'Checked out successfully!', 'success');
                    // Refresh status after checkout
                    setTimeout(() => {
                        this.getRoomStatus().then(status => {
                            this.updateWidgetStatus(widget, status);
                        });
                    }, 1000);
                }).catch(error => {
                    this.showWidgetMessage(widget, error.message || 'Check-out failed', 'error');
                }).finally(() => {
                    checkoutBtn.disabled = false;
                });
            });
        }
    }

    /**
     * Update widget status display
     * @private
     */
    updateWidgetStatus(widget, status) {
        const statusIndicator = widget.querySelector('#status-indicator');
        const roomStatus = widget.querySelector('#room-status');
        const roomOccupied = widget.querySelector('#room-occupied');
        const lastUpdate = widget.querySelector('#last-update');

        if (statusIndicator) {
            if (status.isBooked) {
                statusIndicator.textContent = '🔴';
                statusIndicator.title = 'Room is booked';
            } else {
                statusIndicator.textContent = '🟢';
                statusIndicator.title = 'Room is available';
            }
        }

        if (roomStatus) {
            roomStatus.textContent = status.roomStatus || 'Unknown';
        }

        if (roomOccupied) {
            roomOccupied.textContent = status.isBooked ? 'Yes' : 'No';
        }

        if (lastUpdate) {
            lastUpdate.textContent = new Date().toLocaleTimeString();
        }
    }

    /**
     * Show message in widget
     * @private
     */
    showWidgetMessage(widget, message, type = 'info') {
        const messageArea = widget.querySelector('#message-area');
        if (messageArea) {
            messageArea.textContent = message;
            messageArea.className = `message-area message-${type}`;
            
            // Auto-clear success messages
            if (type === 'success') {
                setTimeout(() => {
                    messageArea.textContent = '';
                    messageArea.className = 'message-area';
                }, 3000);
            }
        }
    }

    /**
     * Cleanup and destroy the controller
     */
    destroy() {
        this.stopStatusPolling();
        this.statusCallbacks = [];
        this.errorCallbacks = [];
        console.log('🗑️  Arduino Controller destroyed');
    }
}

// Export for use in other scripts
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ArduinoController;
}