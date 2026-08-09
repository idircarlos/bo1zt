# Twitch Integration

bo1zt connects to your own Twitch channel to read and write to the chat. It cannot
change your stream, your account or your channel settings.

## Create your Twitch application

1. Go to [dev.twitch.tv/console/apps](https://dev.twitch.tv/console/apps) and log
   in with your Twitch account.
2. Click **Register Your Application**.
3. Fill the form:
   - **Name**: anything free, for example `bo1zt-yourname`.
   - **OAuth Redirect URLs**: `http://localhost`.
   - **Category**: `Game Integration`.
   - **Client Type**: `Public`.
4. Click **Create**, then **Manage** on the application you just created.
5. Copy the **Client ID**.

## Connect

1. Paste the Client ID in the **Client ID** field and click **Connect**.
2. Your browser opens the Twitch activation page. Check that the code shown there
   matches the one in this window and authorize the application.
3. Once authorized, the window shows your account and **Status: Connected as <user>**.
