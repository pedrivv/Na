import discord
from discord import app_commands
from discord.ext import commands
import sqlite3
import os
import json
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()
TOKEN = os.getenv('DISCORD_TOKEN')

# ⚙️ الإعدادات
CONFIG = {
    'allowed_category_id': 1445488217305645138,           # ⬅️ ID كاتيجوري التذاكر
    'bot_channel_id': 1445509977186893905,                # ⬅️ القناة التي يرسل فيها للبوت الآخر
    'bot_command': '/bypass add uid:{uid} days:3',  # ⬅️ أمر البوت الآخر
    'uid_min': 6,                               # ⬅️ أقل عدد أرقام
    'uid_max': 18,                              # ⬅️ أكثر عدد أرقام
    'cooldown_days': 30,                        # ⬅️ أيام الانتظار بين الاستخدامات (0 = مرة واحدة مدى الحياة)
    'database_file': 'uid_tickets.db'           # ⬅️ ملف قاعدة البيانات
}

# إنشاء البوت مع Slash Commands
intents = discord.Intents.default()
intents.message_content = True
intents.members = True

bot = commands.Bot(command_prefix='!', intents=intents)

# ========== قاعدة البيانات ==========

def init_database():
    """تهيئة قاعدة البيانات"""
    conn = sqlite3.connect(CONFIG['database_file'])
    c = conn.cursor()
    
    # جدول المستخدمين
    c.execute('''CREATE TABLE IF NOT EXISTS users
                 (user_id INTEGER PRIMARY KEY, 
                  used_count INTEGER DEFAULT 0,
                  last_used TIMESTAMP,
                  total_uids TEXT DEFAULT '[]')''')
    
    # جدول الـ UIDs
    c.execute('''CREATE TABLE IF NOT EXISTS uids
                 (uid TEXT PRIMARY KEY,
                  user_id INTEGER,
                  used_at TIMESTAMP,
                  channel_id INTEGER)''')
    
    conn.commit()
    conn.close()
    print("✅ قاعدة البيانات جاهزة")

def can_user_use(user_id):
    """تحقق إذا كان المستخدم يستطيع استخدام النظام"""
    conn = sqlite3.connect(CONFIG['database_file'])
    c = conn.cursor()
    
    # جلب بيانات المستخدم
    c.execute('SELECT used_count, last_used FROM users WHERE user_id = ?', (user_id,))
    result = c.fetchone()
    
    if not result:
        # مستخدم جديد
        conn.close()
        return True, "مستخدم جديد"
    
    used_count, last_used = result
    
    # إذا كان 0 = مرة واحدة مدى الحياة
    if CONFIG['cooldown_days'] == 0:
        if used_count > 0:
            conn.close()
            return False, "لقد استخدمت فرصتك الوحيدة بالفعل!"
        else:
            conn.close()
            return True, "مستخدم جديد - فرصة أولى"
    
    # إذا كان هناك فترة تبريد
    if last_used:
        last_used_date = datetime.fromisoformat(last_used)
        days_passed = (datetime.now() - last_used_date).days
        
        if days_passed < CONFIG['cooldown_days']:
            days_left = CONFIG['cooldown_days'] - days_passed
            conn.close()
            return False, f"انتظر {days_left} يوم/أيام قبل الاستخدام التالي"
    
    conn.close()
    return True, "مسموح بالاستخدام"

def add_usage(user_id, uid, channel_id):
    """إضافة استخدام جديد"""
    conn = sqlite3.connect(CONFIG['database_file'])
    c = conn.cursor()
    
    now = datetime.now().isoformat()
    
    # تحديث بيانات المستخدم
    c.execute('''INSERT OR REPLACE INTO users (user_id, used_count, last_used, total_uids)
                 VALUES (?, 
                         COALESCE((SELECT used_count FROM users WHERE user_id = ?), 0) + 1,
                         ?,
                         COALESCE((SELECT total_uids FROM users WHERE user_id = ?), '[]')
                )''', 
              (user_id, user_id, now, user_id))
    
    # تحديث الـ UID
    c.execute('INSERT OR REPLACE INTO uids (uid, user_id, used_at, channel_id) VALUES (?, ?, ?, ?)',
              (uid, user_id, now, channel_id))
    
    conn.commit()
    
    # تحديث قائمة UIDs
    c.execute('SELECT total_uids FROM users WHERE user_id = ?', (user_id,))
    result = c.fetchone()
    if result:
        uids_list = json.loads(result[0])
        uids_list.append(uid)
        c.execute('UPDATE users SET total_uids = ? WHERE user_id = ?', 
                  (json.dumps(uids_list), user_id))
    
    conn.commit()
    conn.close()

def get_user_stats(user_id):
    """جلب إحصائيات المستخدم"""
    conn = sqlite3.connect(CONFIG['database_file'])
    c = conn.cursor()
    
    c.execute('SELECT used_count, last_used, total_uids FROM users WHERE user_id = ?', (user_id,))
    result = c.fetchone()
    
    if not result:
        conn.close()
        return None
    
    used_count, last_used, total_uids_json = result
    total_uids = json.loads(total_uids_json) if total_uids_json else []
    
    conn.close()
    
    return {
        'used_count': used_count,
        'last_used': last_used,
        'total_uids': total_uids,
        'total_used': len(total_uids)
    }

# ========== Slash Commands ==========

@bot.tree.command(name="uid", description="إضافة UID للبوت الآخر (فقط في تذاكر)")
@app_commands.describe(uid="الرقم السري UID (6-18 رقم)")
async def uid_slash(interaction: discord.Interaction, uid: str):
    """أمر Slash لإضافة UID"""
    
    # التحقق 1: أن تكون في تذكرة (كاتيجوري التذاكر)
    if not interaction.channel.category or interaction.channel.category.id != CONFIG['allowed_category_id']:
        embed = discord.Embed(
            title="❌ قناة غير صالحة",
            description="**هذا الأمر يعمل فقط في التذاكر!**\n\nافتح تذكرة ثم استخدم الأمر.",
            color=0xff0000
        )
        await interaction.response.send_message(embed=embed, ephemeral=True)
        return
    
    # التحقق 2: أن UID أرقام فقط
    if not uid.isdigit():
        embed = discord.Embed(
            title="❌ خطأ في المدخلات",
            description="**الـ UID يجب أن يحتوي على أرقام فقط!**",
            color=0xff0000
        )
        await interaction.response.send_message(embed=embed, ephemeral=True)
        return
    
    # التحقق 3: طول UID
    uid_length = len(uid)
    if not (CONFIG['uid_min'] <= uid_length <= CONFIG['uid_max']):
        embed = discord.Embed(
            title="❌ خطأ في الطول",
            description=f"**الـ UID يجب أن يكون بين {CONFIG['uid_min']} و {CONFIG['uid_max']} رقم!**",
            color=0xff0000
        )
        await interaction.response.send_message(embed=embed, ephemeral=True)
        return
    
    # التحقق 4: إذا كان المستخدم يستطيع الاستخدام
    can_use, message = can_user_use(interaction.user.id)
    if not can_use:
        embed = discord.Embed(
            title="❌ غير مسموح",
            description=f"**{message}**",
            color=0xff9900
        )
        await interaction.response.send_message(embed=embed, ephemeral=True)
        return
    
    # التحقق 5: أن القناة الهدف موجودة
    target_channel = bot.get_channel(CONFIG['bot_channel_id'])
    if not target_channel:
        embed = discord.Embed(
            title="❌ خطأ في النظام",
            description="**القناة الهدف غير موجودة!**",
            color=0xff0000
        )
        await interaction.response.send_message(embed=embed, ephemeral=True)
        return
    
    # إرسال رسالة "جاري المعالجة"
    embed = discord.Embed(
        title="🔄 جاري المعالجة",
        description=f"**جاري إرسال الـ UID:** `{uid}`\n\n⏳ الرجاء الانتظار...",
        color=0xffff00
    )
    await interaction.response.send_message(embed=embed, ephemeral=False)
    
    try:
        # إرسال الأمر للبوت الآخر
        bot_command = CONFIG['bot_command'].format(uid=uid)
        await target_channel.send(bot_command)
        
        # حفظ الاستخدام في قاعدة البيانات
        add_usage(interaction.user.id, uid, interaction.channel.id)
        
        # تحديث الرسالة بنجاح الإرسال
        stats = get_user_stats(interaction.user.id)
        
        success_embed = discord.Embed(
            title="✅ تم الإرسال بنجاح",
            description=f"**تم إرسال الـ UID للبوت الآخر**",
            color=0x00ff00
        )
        
        success_embed.add_field(name="📝 الـ UID", value=f"`{uid}`", inline=True)
        success_embed.add_field(name="📊 الطول", value=f"{uid_length} رقم", inline=True)
        success_embed.add_field(name="👤 المستخدم", value=interaction.user.mention, inline=True)
        
        if CONFIG['cooldown_days'] == 0:
            usage_text = "✅ تم استخدام فرصتك الوحيدة"
        else:
            usage_text = f"📊 الاستخدامات: {stats['used_count'] if stats else 1}"
        
        success_embed.add_field(name="📈 حالة الاستخدام", value=usage_text, inline=False)
        success_embed.add_field(name="🤖 الأمر المرسل", value=f"```{bot_command}```", inline=False)
        
        success_embed.set_footer(text=f"الوقت: {datetime.now().strftime('%H:%M:%S')}")
        
        # تحرير الرسالة الأصلية
        message = await interaction.original_response()
        await message.edit(embed=success_embed)
        
        # تسجيل في الكونسول
        print(f"✅ UID Sent: {uid} | User: {interaction.user.name} | Channel: {interaction.channel.name}")
        
    except Exception as e:
        # في حالة خطأ
        error_embed = discord.Embed(
            title="❌ فشل الإرسال",
            description=f"**حدث خطأ:**\n```{str(e)}```",
            color=0xff0000
        )
        message = await interaction.original_response()
        await message.edit(embed=error_embed)

@bot.tree.command(name="uid_stats", description="عرض إحصائياتك مع النظام")
async def uid_stats(interaction: discord.Interaction):
    """عرض إحصائيات المستخدم"""
    
    stats = get_user_stats(interaction.user.id)
    
    if not stats or stats['used_count'] == 0:
        embed = discord.Embed(
            title="📊 إحصائياتك",
            description="**لم تستخدم النظام بعد!**\n\nاستخدم `/uid` في تذكرة لإضافة أول UID.",
            color=0x7289da
        )
    else:
        embed = discord.Embed(
            title="📊 إحصائياتك",
            description=f"**إحصائيات استخدام النظام**",
            color=0x7289da
        )
        
        embed.add_field(name="📈 عدد الاستخدامات", value=stats['used_count'], inline=True)
        embed.add_field(name="🔢 عدد الـ UIDs", value=stats['total_used'], inline=True)
        
        if stats['last_used']:
            last_used_date = datetime.fromisoformat(stats['last_used'])
            embed.add_field(name="🕒 آخر استخدام", value=last_used_date.strftime("%Y-%m-%d %H:%M"), inline=True)
        
        if CONFIG['cooldown_days'] == 0:
            remaining = "⛔ فرصة واحدة مدى الحياة (مستخدمة)" if stats['used_count'] > 0 else "✅ فرصة واحدة متبقية"
        else:
            if stats['last_used']:
                last_used_date = datetime.fromisoformat(stats['last_used'])
                days_passed = (datetime.now() - last_used_date).days
                days_left = max(0, CONFIG['cooldown_days'] - days_passed)
                remaining = f"⏳ {days_left} يوم/أيام متبقية"
            else:
                remaining = "✅ جاهز للاستخدام"
        
        embed.add_field(name="🔄 المتبقي", value=remaining, inline=True)
        
        # عرض آخر 5 UIDs
        if stats['total_uids']:
            recent_uids = stats['total_uids'][-5:]  # آخر 5
            uids_text = "\n".join([f"`{uid}`" for uid in recent_uids])
            embed.add_field(name="📝 آخر الـ UIDs", value=uids_text, inline=False)
    
    embed.set_footer(text=f"المستخدم: {interaction.user.name}")
    await interaction.response.send_message(embed=embed, ephemeral=True)

@bot.tree.command(name="uid_help", description="شرح كيفية استخدام النظام")
async def uid_help(interaction: discord.Interaction):
    """شرح النظام"""
    
    embed = discord.Embed(
        title="📚 نظام UID للتذاكر",
        description="**كيفية استخدام النظام:**",
        color=0x00ffff
    )
    
    embed.add_field(
        name="📋 الخطوات",
        value="1. افتح تذكرة جديدة\n"
              "2. استخدم الأمر `/uid <الرقم>`\n"
              "3. أدخل الـ UID (6-18 رقم)\n"
              "4. انتظر التأكيد",
        inline=False
    )
    
    embed.add_field(
        name="⚙️ المتطلبات",
        value=f"• UID: أرقام فقط\n"
              f"• الطول: {CONFIG['uid_min']}-{CONFIG['uid_max']} رقم\n"
              f"• المكان: تذاكر فقط\n"
              f"• الفرص: {'مرة واحدة مدى الحياة' if CONFIG['cooldown_days'] == 0 else f'كل {CONFIG['cooldown_days']} يوم'}",
        inline=False
    )
    
    embed.add_field(
        name="🔧 الأوامر",
        value="`/uid <رقم>` - إضافة UID\n"
              "`/uid_stats` - إحصائياتك\n"
              "`/uid_help` - هذه المساعدة",
        inline=False
    )
    
    await interaction.response.send_message(embed=embed, ephemeral=True)

# ========== أوامر نصية قديمة للتوافق ==========

@bot.command(name='uid')
async def uid_text(ctx, uid: str = None):
    """نسخة نصية من الأمر للتوافق"""
    
    # نفس التحقق من Slash Command
    if not ctx.channel.category or ctx.channel.category.id != CONFIG['allowed_category_id']:
        await ctx.send("❌ هذا الأمر يعمل فقط في التذاكر!", delete_after=5)
        return
    
    if not uid:
        await ctx.send(f"❌ استخدم: `!uid <الرقم>`\nمثال: `!uid 123456789`", delete_after=5)
        return
    
    if not uid.isdigit():
        await ctx.send("❌ الـ UID يجب أن يكون أرقام فقط!", delete_after=5)
        return
    
    uid_length = len(uid)
    if not (CONFIG['uid_min'] <= uid_length <= CONFIG['uid_max']):
        await ctx.send(f"❌ UID يجب أن يكون بين {CONFIG['uid_min']} و {CONFIG['uid_max']} رقم!", delete_after=5)
        return
    
    can_use, message = can_user_use(ctx.author.id)
    if not can_use:
        await ctx.send(f"❌ {message}", delete_after=10)
        return
    
    target_channel = bot.get_channel(CONFIG['bot_channel_id'])
    if not target_channel:
        await ctx.send("❌ خطأ في النظام!", delete_after=5)
        return
    
    # إرسال
    bot_command = CONFIG['bot_command'].format(uid=uid)
    await target_channel.send(bot_command)
    add_usage(ctx.author.id, uid, ctx.channel.id)
    
    await ctx.send(f"✅ تم إرسال UID: `{uid}` للبوت الآخر")

# ========== أوامر الإدارة (للمشرفين) ==========

@bot.tree.command(name="uid_admin", description="أوامر الإدارة (للمشرفين فقط)")
@app_commands.describe(action="الإجراء", user="المستخدم")
@app_commands.choices(action=[
    app_commands.Choice(name="عرض_إحصائيات", value="stats"),
    app_commands.Choice(name="إعادة_تعيين", value="reset"),
    app_commands.Choice(name="قائمة_المستخدمين", value="list"),
    app_commands.Choice(name="عرض_البيانات", value="view")
])
async def uid_admin(interaction: discord.Interaction, action: str, user: discord.User = None):
    """أوامر إدارة النظام"""
    
    # التحقق من صلاحيات المشرف
    if not interaction.user.guild_permissions.administrator:
        await interaction.response.send_message("❌ تحتاج صلاحية المشرف!", ephemeral=True)
        return
    
    if action == "stats":
        # إحصائيات عامة
        conn = sqlite3.connect(CONFIG['database_file'])
        c = conn.cursor()
        
        c.execute('SELECT COUNT(*) FROM users')
        total_users = c.fetchone()[0]
        
        c.execute('SELECT COUNT(*) FROM uids')
        total_uids = c.fetchone()[0]
        
        c.execute('SELECT COUNT(*) FROM users WHERE used_count > 0')
        active_users = c.fetchone()[0]
        
        conn.close()
        
        embed = discord.Embed(
            title="📊 إحصائيات النظام",
            color=0x00ff00
        )
        
        embed.add_field(name="👥 إجمالي المستخدمين", value=total_users, inline=True)
        embed.add_field(name="✅ المستخدمين النشطين", value=active_users, inline=True)
        embed.add_field(name="🔢 إجمالي الـ UIDs", value=total_uids, inline=True)
        embed.add_field(name="⚙️ إعدادات النظام", 
                       value=f"الطول: {CONFIG['uid_min']}-{CONFIG['uid_max']}\n"
                             f"الفرص: {'مرة واحدة' if CONFIG['cooldown_days'] == 0 else f'كل {CONFIG['cooldown_days']} يوم'}",
                       inline=False)
        
        await interaction.response.send_message(embed=embed, ephemeral=True)
    
    elif action == "reset" and user:
        # إعادة تعيين مستخدم
        conn = sqlite3.connect(CONFIG['database_file'])
        c = conn.cursor()
        
        c.execute('DELETE FROM users WHERE user_id = ?', (user.id,))
        c.execute('DELETE FROM uids WHERE user_id = ?', (user.id,))
        
        conn.commit()
        conn.close()
        
        embed = discord.Embed(
            title="🔄 تمت إعادة التعيين",
            description=f"تم إعادة تعيين بيانات {user.mention}",
            color=0x00ff00
        )
        await interaction.response.send_message(embed=embed, ephemeral=True)
    
    elif action == "view" and user:
        # عرض بيانات مستخدم
        stats = get_user_stats(user.id)
        
        if not stats:
            await interaction.response.send_message(f"❌ {user.name} ليس لديه بيانات!", ephemeral=True)
            return
        
        embed = discord.Embed(
            title=f"📋 بيانات {user.name}",
            color=0x7289da
        )
        
        embed.add_field(name="📈 عدد الاستخدامات", value=stats['used_count'], inline=True)
        embed.add_field(name="🔢 عدد الـ UIDs", value=stats['total_used'], inline=True)
        
        if stats['last_used']:
            last_used = datetime.fromisoformat(stats['last_used'])
            embed.add_field(name="🕒 آخر استخدام", value=last_used.strftime("%Y-%m-%d %H:%M"), inline=True)
        
        # عرض جميع UIDs
        if stats['total_uids']:
            uids_text = "\n".join([f"`{uid}`" for uid in stats['total_uids'][-10:]])
            embed.add_field(name="📝 الـ UIDs", value=uids_text[:1000] + ("..." if len(uids_text) > 1000 else ""), inline=False)
        
        await interaction.response.send_message(embed=embed, ephemeral=True)

# ========== الأحداث ==========

@bot.event
async def on_ready():
    # تهيئة قاعدة البيانات
    init_database()
    
    print(f"✅ البوت يعمل: {bot.user}")
    print(f"📊 نظام تذاكر UID جاهز")
    print(f"   - الكاتيجوري: {CONFIG['allowed_category_id']}")
    print(f"   - القناة الهدف: {CONFIG['bot_channel_id']}")
    print(f"   - الفرص: {'مرة واحدة مدى الحياة' if CONFIG['cooldown_days'] == 0 else f'كل {CONFIG['cooldown_days']} يوم'}")
    
    # مزامنة Slash Commands
    try:
        synced = await bot.tree.sync()
        print(f"✅ تم مزامنة {len(synced)} أمر")
    except Exception as e:
        print(f"❌ خطأ في المزامنة: {e}")
    
    # تغيير حالة البوت
    await bot.change_presence(
        activity=discord.Activity(
            type=discord.ActivityType.watching,
            name="/uid في التذاكر"
        )
    )

# ========== تشغيل البوت ==========

if __name__ == "__main__":
    print("🚀 جاري تشغيل بوت تذاكر UID...")
    bot.run(TOKEN)